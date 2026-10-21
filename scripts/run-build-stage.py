"""Run an owned Windows build stage hidden, with Job Object family metrics.

The process starts suspended, is assigned to the job, then resumes. No child
environment values are persisted. Peak memory is committed job memory, not RSS.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import datetime
import json
import msvcrt
import os
from pathlib import Path
import subprocess
import sys
import time
import _winapi


class BasicLimits(c.Structure):
    _fields_ = [("ProcessUserTime", c.c_int64), ("JobUserTime", c.c_int64), ("Flags", w.DWORD), ("MinWorkingSet", c.c_size_t), ("MaxWorkingSet", c.c_size_t), ("ActiveProcessLimit", w.DWORD), ("Affinity", c.c_size_t), ("Priority", w.DWORD), ("Scheduling", w.DWORD)]


class IoCounters(c.Structure):
    _fields_ = [(n, c.c_uint64) for n in ["ReadOps", "WriteOps", "OtherOps", "ReadBytes", "WriteBytes", "OtherBytes"]]


class ExtendedLimits(c.Structure):
    _fields_ = [("Basic", BasicLimits), ("Io", IoCounters), ("ProcessMemoryLimit", c.c_size_t), ("JobMemoryLimit", c.c_size_t), ("PeakProcessMemory", c.c_size_t), ("PeakJobMemory", c.c_size_t)]


class Accounting(c.Structure):
    _fields_ = [("UserTime", c.c_int64), ("KernelTime", c.c_int64), ("PeriodUserTime", c.c_int64), ("PeriodKernelTime", c.c_int64), ("PageFaults", w.DWORD), ("TotalProcesses", w.DWORD), ("ActiveProcesses", w.DWORD), ("TerminatedProcesses", w.DWORD)]


K = c.WinDLL("kernel32", use_last_error=True)
K.CreateJobObjectW.argtypes = [c.c_void_p, w.LPCWSTR]
K.CreateJobObjectW.restype = w.HANDLE
K.SetInformationJobObject.argtypes = [w.HANDLE, c.c_int, c.c_void_p, w.DWORD]
K.SetInformationJobObject.restype = w.BOOL
K.QueryInformationJobObject.argtypes = [w.HANDLE, c.c_int, c.c_void_p, w.DWORD, c.c_void_p]
K.QueryInformationJobObject.restype = w.BOOL
K.AssignProcessToJobObject.argtypes = [w.HANDLE, w.HANDLE]
K.AssignProcessToJobObject.restype = w.BOOL
K.ResumeThread.argtypes = [w.HANDLE]
K.ResumeThread.restype = w.DWORD
K.TerminateJobObject.argtypes = [w.HANDLE, w.UINT]
K.TerminateJobObject.restype = w.BOOL
K.CloseHandle.argtypes = [w.HANDLE]
K.CloseHandle.restype = w.BOOL


def checked(ok):
    if not ok:
        raise c.WinError(c.get_last_error())


def query(job, kind, value):
    checked(K.QueryInformationJobObject(job, kind, c.byref(value), c.sizeof(value), None))
    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cwd", required=True)
    parser.add_argument("--log-prefix", required=True)
    parser.add_argument("--timeout", type=float, default=180)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    argv = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not argv:
        parser.error("a command is required")
    prefix = Path(args.log_prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    report = {"argv": argv, "cwd": args.cwd, "started_at": datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=9))).isoformat(), "hidden": True, "environment_persisted_by_runner": False, "memory_definition": "Job Object PeakJobMemoryUsed: peak simultaneous committed memory for the assigned process and descendants; not resident working set."}
    job = K.CreateJobObjectW(None, None)
    checked(job)
    process = thread = None
    start = time.perf_counter()
    try:
        limits = ExtendedLimits()
        limits.Basic.Flags = 0x2000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
        checked(K.SetInformationJobObject(job, 9, c.byref(limits), c.sizeof(limits)))
        with prefix.with_suffix(".stdout.log").open("xb") as stdout, prefix.with_suffix(".stderr.log").open("xb") as stderr, open(os.devnull, "rb") as stdin:
            handles = [msvcrt.get_osfhandle(f.fileno()) for f in (stdout, stderr, stdin)]
            for h in handles:
                os.set_handle_inheritable(h, True)
            startup = subprocess.STARTUPINFO()
            startup.dwFlags |= subprocess.STARTF_USESTDHANDLES
            startup.hStdOutput, startup.hStdError, startup.hStdInput = handles
            process, thread, pid, tid = _winapi.CreateProcess(argv[0], subprocess.list2cmdline(argv), None, None, True, 0x4 | subprocess.CREATE_NO_WINDOW, None, args.cwd, startup)
            report["pid"] = pid
            try:
                checked(K.AssignProcessToJobObject(job, process))
            except Exception:
                _winapi.TerminateProcess(process, 1)
                raise
            if K.ResumeThread(thread) == 0xFFFFFFFF:
                raise c.WinError(c.get_last_error())
            _winapi.CloseHandle(thread)
            thread = None
            deadline = start + args.timeout
            while _winapi.WaitForSingleObject(process, 20) != _winapi.WAIT_OBJECT_0:
                if time.perf_counter() > deadline:
                    raise TimeoutError(f"Owned process family exceeded {args.timeout} seconds")
            report["exit_code"] = _winapi.GetExitCodeProcess(process)
        # Console hosts can outlive the completed command while inherited handles
        # are still open. Close redirection handles first, allow a short final
        # drain, then terminate only residual members of this owned job.
        drain_deadline = min(deadline, time.perf_counter() + 2)
        accounting = query(job, 1, Accounting())
        while accounting.ActiveProcesses and time.perf_counter() < drain_deadline:
            time.sleep(0.02)
            accounting = query(job, 1, Accounting())
        report["residual_owned_processes_terminated"] = accounting.ActiveProcesses
        if accounting.ActiveProcesses:
            checked(K.TerminateJobObject(job, 0))
            while accounting.ActiveProcesses and time.perf_counter() < deadline:
                time.sleep(0.02)
                accounting = query(job, 1, Accounting())
            if accounting.ActiveProcesses:
                raise RuntimeError("Owned job cleanup did not reach zero active processes")
        report["cpu_user_seconds"] = accounting.UserTime / 10000000
        report["cpu_kernel_seconds"] = accounting.KernelTime / 10000000
        report["cpu_family_seconds"] = (accounting.UserTime + accounting.KernelTime) / 10000000
        report["total_family_processes"] = accounting.TotalProcesses
        report["active_family_processes_at_completion"] = accounting.ActiveProcesses
        report["peak_family_committed_bytes"] = query(job, 9, ExtendedLimits()).PeakJobMemory
    except Exception as exc:
        report["error"] = str(exc)
        report["exit_code"] = 1
    finally:
        report["elapsed_seconds"] = time.perf_counter() - start
        checked(K.CloseHandle(job))  # Kills any still-owned members on timeout/error.
        if thread is not None:
            _winapi.CloseHandle(thread)
        if process is not None:
            _winapi.CloseHandle(process)
        prefix.with_suffix(".metrics.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report), flush=True)
    return report["exit_code"]


if __name__ == "__main__":
    sys.exit(main())
