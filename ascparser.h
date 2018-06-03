#ifndef BLIFPARSER_H
#define BLIFPARSER_H

#include <QIODevice>
#include <QRegularExpression>

class AscParser
{
public:
    AscParser(QIODevice *in);

    bool isOk() const;
    bool atEnd() const;

    bool atEol();
    void parseEol();

    bool atCommand();
    void skipToCommand();
    QString parseCommand();

    int parseDecimal();
    int parseBinary();
    QString parseName();

private:
    QIODevice *_in;
    QString _line;
    int _lineno;
    int _pos;
    bool _error;

    void refill();
    QRegularExpressionMatch matchRegexp(const QRegularExpression &re);
    QRegularExpressionMatch matchToken(const QRegularExpression &re);
};

#endif // BLIFPARSER_H
