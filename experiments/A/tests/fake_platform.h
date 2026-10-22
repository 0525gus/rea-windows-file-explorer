#pragma once
#include "explorer/platform.h"
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

class FakePlatform final : public explorer::Platform {
  public:
    struct Answer {
        explorer::Result<explorer::Listing> result;
        int delayMs = 0;
        bool ignoreCancel = false;
    };
    mutable QMutex mutex;
    QHash<QString, Answer> answers;
    mutable QStringList opened;
    explorer::Capabilities capabilities() const override {
        return {};
    }
    static QString key(const explorer::Location &location) {
        return location.computer ? QStringLiteral(":computer:") : location.path;
    }
    void set(const explorer::Location &location, explorer::Result<explorer::Listing> result, int delay = 0,
             bool ignoreCancel = false) {
        QMutexLocker lock(&mutex);
        answers.insert(key(location), {std::move(result), delay, ignoreCancel});
    }
    explorer::Result<explorer::Listing> list(const explorer::Location &location,
                                             const explorer::CancelToken &cancel) const override {
        Answer answer{explorer::Listing{location, {}, {}}, 0, false};
        {
            QMutexLocker lock(&mutex);
            if (answers.contains(key(location)))
                answer = answers.value(key(location));
        }
        for (int waited = 0; waited < answer.delayMs; waited += 5) {
            if (!answer.ignoreCancel && cancel->load())
                return explorer::cancelled(location.path);
            QThread::msleep(5);
        }
        return answer.result;
    }
    explorer::Result<explorer::Success> openFile(const QString &path) const override {
        QMutexLocker lock(&mutex);
        opened.push_back(path);
        return explorer::Success{};
    }
};
