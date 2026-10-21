#include "explorer/types.hpp"
#include <QRegularExpression>
#include <QTest>
#include <algorithm>
#include <limits>

using namespace explorer;

namespace {
Location location(const QString &key) {
    Location value;
    value.kind = Location::Kind::Directory;
    value.callerInput = key;
    value.path = QStringLiteral("/fixture/") + key;
    value.key = key;
    value.label = key;
    return value;
}

Entry entry(QString name, QString key, bool directory = false, std::optional<quint64> size = {},
            std::optional<qint64> modified = {}) {
    Entry value;
    value.name = std::move(name);
    value.key = std::move(key);
    value.path = QStringLiteral("/fixture/") + value.name;
    value.directory = directory;
    value.sizeBytes = size;
    value.modifiedMs = modified;
    return value;
}

// Small test comparator with an independent numeric oracle. Real OS natural
// comparison is tested in platform_tests, rather than assumed from this double.
int naturalCompare(const QString &a, const QString &b) {
    static const QRegularExpression token(QStringLiteral("(\\d+)|(\\D+)"));
    auto left = token.globalMatch(a.toCaseFolded());
    auto right = token.globalMatch(b.toCaseFolded());
    while (left.hasNext() && right.hasNext()) {
        const auto l = left.next();
        const auto r = right.next();
        int compared;
        if (!l.captured(1).isEmpty() && !r.captured(1).isEmpty()) {
            const auto ln = l.captured(1).toULongLong();
            const auto rn = r.captured(1).toULongLong();
            compared = ln < rn ? -1 : (ln > rn ? 1 : 0);
        } else {
            compared = QString::compare(l.captured(), r.captured(), Qt::CaseSensitive);
        }
        if (compared)
            return compared;
    }
    return left.hasNext() ? 1 : (right.hasNext() ? -1 : 0);
}

QStringList keys(const QVector<Entry> &values) {
    QStringList result;
    for (const auto &value : values)
        result.push_back(value.key);
    return result;
}
} // namespace

class DomainTests final : public QObject {
    Q_OBJECT
  private slots:
    void historyBoundariesAndSuccessfulBranch() {
        History history;
        QCOMPARE(history.cursor(), -1);
        QVERIFY(!history.canBack());
        QVERIFY(!history.canForward());
        QVERIFY(!history.at(-1));
        QVERIFY(!history.at(0));
        QVERIFY(!history.commitCursor(0));

        history.visit(location(QStringLiteral("A")));
        history.visit(location(QStringLiteral("B")));
        history.visit(location(QStringLiteral("C")));
        QCOMPARE(history.cursor(), 2);
        QVERIFY(history.canBack());
        QVERIFY(!history.canForward());
        QVERIFY(history.commitCursor(1));
        QCOMPARE(history.at(history.cursor())->key, QStringLiteral("B"));
        QVERIFY(history.canForward());
        QVERIFY(!history.commitCursor(10));
        QCOMPARE(history.cursor(), 1);
        history.visit(location(QStringLiteral("D")));
        QCOMPARE(history.locations().size(), 3);
        QCOMPARE(history.at(0)->key, QStringLiteral("A"));
        QCOMPARE(history.at(1)->key, QStringLiteral("B"));
        QCOMPARE(history.at(2)->key, QStringLiteral("D"));
        QVERIFY(!history.canForward());
    }

    void repeatedLocationUsesIdentityWithoutLosingForwardBranch() {
        History history;
        history.visit(location(QStringLiteral("A")));
        history.visit(location(QStringLiteral("B")));
        QVERIFY(history.commitCursor(0));
        auto sameIdentity = location(QStringLiteral("A"));
        sameIdentity.callerInput = QStringLiteral("different spelling of A");
        sameIdentity.label = QStringLiteral("different display");
        history.visit(sameIdentity);
        QCOMPARE(history.cursor(), 0);
        QCOMPARE(history.locations().size(), 2);
        QVERIFY(history.canForward());
    }

    void unicodeLiteralSearchPreservesNamesAndMetadata() {
        const QVector<Entry> original{
            entry(QString::fromUtf8("한글_파일.TXT"), QStringLiteral("korean"), false, 17, 42),
            entry(QString::fromUtf8("Straße.txt"), QStringLiteral("folded")),
            entry(QString::fromUtf8("café.txt"), QStringLiteral("accented")),
            entry(QStringLiteral("Report[1].txt"), QStringLiteral("literal")),
            entry(QStringLiteral("Report1.txt"), QStringLiteral("plain")),
            entry(QStringLiteral("two words.txt"), QStringLiteral("space"))};
        QCOMPARE(keys(filterEntries(original, QString::fromUtf8("한글"))),
                 QStringList{QStringLiteral("korean")});
        QCOMPARE(keys(filterEntries(original, QString::fromUtf8("STRAßE"))),
                 QStringList{QStringLiteral("folded")});
        QCOMPARE(keys(filterEntries(original, QString::fromUtf8("CAFÉ"))),
                 QStringList{QStringLiteral("accented")});
        // The declared Qt toCaseFolded literal-search contract does not promise
        // multi-character transliteration (ß -> ss). Qt6.8.3 preserved ß in the
        // actual diagnostic; retain that boundary rather than require a new API.
        QCOMPARE(keys(filterEntries(original, QStringLiteral("STRASSE"))), QStringList{});
        QCOMPARE(keys(filterEntries(original, QStringLiteral("[1]"))),
                 QStringList{QStringLiteral("literal")});
        QCOMPARE(keys(filterEntries(original, QStringLiteral("*"))), QStringList{});
        QCOMPARE(keys(filterEntries(original, QStringLiteral(" "))), QStringList{QStringLiteral("space")});
        QCOMPARE(filterEntries(original, {}).size(), original.size());
        const auto result = filterEntries(original, QStringLiteral(".txt"));
        QCOMPARE(result.size(), original.size());
        QCOMPARE(result.first().name, original.first().name);
        QCOMPARE(result.first().sizeBytes, original.first().sizeBytes);
        QCOMPARE(result.first().modifiedMs, original.first().modifiedMs);
        QCOMPARE(original.first().name, QString::fromUtf8("한글_파일.TXT"));
    }

    void directoryPriorityAndNaturalNamesBothDirections() {
        const QVector<Entry> input{entry(QStringLiteral("file10"), QStringLiteral("f10")),
                                   entry(QStringLiteral("dir10"), QStringLiteral("d10"), true),
                                   entry(QStringLiteral("file2"), QStringLiteral("f2")),
                                   entry(QStringLiteral("dir2"), QStringLiteral("d2"), true)};
        auto sorted = input;
        sortEntries(sorted, {SortColumn::Name, true}, naturalCompare);
        QCOMPARE(keys(sorted), (QStringList{QStringLiteral("d2"), QStringLiteral("d10"), QStringLiteral("f2"),
                                            QStringLiteral("f10")}));
        sorted = input;
        sortEntries(sorted, {SortColumn::Name, false}, naturalCompare);
        QCOMPARE(keys(sorted), (QStringList{QStringLiteral("d10"), QStringLiteral("d2"),
                                            QStringLiteral("f10"), QStringLiteral("f2")}));
    }

    void metadataSortKeepsUnknownLastAndNaturalTieBreakAscending() {
        const QVector<Entry> input{entry(QStringLiteral("missing"), QStringLiteral("unknown")),
                                   entry(QStringLiteral("item10"), QStringLiteral("tie10"), false, 9, 100),
                                   entry(QStringLiteral("directory"), QStringLiteral("directory"), true),
                                   entry(QStringLiteral("item2"), QStringLiteral("tie2"), false, 9, 100),
                                   entry(QStringLiteral("zero"), QStringLiteral("zero"), false, 0, 0),
                                   entry(QStringLiteral("largest"), QStringLiteral("largest"), false,
                                         std::numeric_limits<quint64>::max(),
                                         std::numeric_limits<qint64>::max())};
        for (const auto column : {SortColumn::Size, SortColumn::Modified}) {
            auto sorted = input;
            sortEntries(sorted, {column, true}, naturalCompare);
            QCOMPARE(keys(sorted), (QStringList{QStringLiteral("directory"), QStringLiteral("zero"),
                                                QStringLiteral("tie2"), QStringLiteral("tie10"),
                                                QStringLiteral("largest"), QStringLiteral("unknown")}));
            sorted = input;
            sortEntries(sorted, {column, false}, naturalCompare);
            QCOMPARE(keys(sorted), (QStringList{QStringLiteral("directory"), QStringLiteral("largest"),
                                                QStringLiteral("tie2"), QStringLiteral("tie10"),
                                                QStringLiteral("zero"), QStringLiteral("unknown")}));
        }
    }

    void tiesUseSourceNameThenKeyIndependentOfInputPermutation() {
        QVector<Entry> input{entry(QStringLiteral("a"), QStringLiteral("z"), false, 1),
                             entry(QStringLiteral("A"), QStringLiteral("c"), false, 1),
                             entry(QStringLiteral("A"), QStringLiteral("b"), false, 1)};
        const QStringList expected{QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("z")};
        for (int shift = 0; shift < input.size(); ++shift) {
            std::rotate(input.begin(), input.begin() + 1, input.end());
            auto result = input;
            sortEntries(result, {SortColumn::Size, false}, naturalCompare);
            QCOMPARE(keys(result), expected);
        }
    }
};

QTEST_GUILESS_MAIN(DomainTests)
#include "domain_tests.moc"
