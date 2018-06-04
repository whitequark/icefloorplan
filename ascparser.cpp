#include <QtDebug>
#include "ascparser.h"

AscParser::AscParser(QIODevice *in) : _in(in), _lineno(-1), _error(false)
{}

bool AscParser::isOk() const
{
    return !_error;
}

bool AscParser::atEnd() const
{
    return _in->bytesAvailable() == 0;
}

void AscParser::refill()
{
    if(_pos < _line.length()) return;
    _pos = 0;

    _line = _in->readLine();
    _lineno++;
    // qDebug() << "read" << _line;
    while(_line == "\n" || _line[0] == '#') {
        _line = _in->readLine();
        _lineno++;
        // qDebug() << "read" << _line;
    }
}

QRegularExpressionMatch AscParser::matchRegexp(const QRegularExpression &re)
{
    refill();

    QRegularExpressionMatch match = re.match(_line, _pos, QRegularExpression::NormalMatch,
                                             QRegularExpression::AnchoredMatchOption);
    if(!match.hasMatch()) {
        _error = true;
        qCritical() << "at line" << _lineno << "cannot match" << re.pattern() << "on"
                    << _line.mid(_pos);
    } else {
        _pos += match.capturedLength();
    }
    return match;
}

QRegularExpressionMatch AscParser::matchToken(const QRegularExpression &re)
{
    QRegularExpressionMatch match = matchRegexp(re);
    if(match.hasMatch()) {
        while(_line[_pos] == ' ' || _line[_pos] == '\n') {
            _pos++;
        }
    }
    return match;
}

bool AscParser::atEol()
{
    return _pos == _line.length();
}

void AscParser::parseEol()
{
    if(_pos < _line.length()) {
        _error = true;
        qCritical() << "at line" << _lineno << "not at end of line:" << _line.mid(_pos)
                    << "remaining";
    }
}

QString AscParser::parseRest()
{
    QString rest = _line.mid(_pos);
    _pos         = _line.length();

    if(rest.endsWith("\n")) rest.chop(1);
    return rest;
}

bool AscParser::atCommand()
{
    refill();
    return atEnd() || (_pos == 0 && _line[_pos] == '.');
}

void AscParser::skipToCommand()
{
    do {
        _pos = _line.length();
        refill();
    } while(!atEnd() && !atCommand());
}

QString AscParser::parseCommand()
{
    static const QRegularExpression re("\\.([a-z0-9_]+)");
    QRegularExpressionMatch match = matchToken(re);
    if(!match.hasMatch()) return "";
    return match.captured(1);
}

uint AscParser::parseBinary()
{
    static const QRegularExpression re("[01]+");
    QRegularExpressionMatch match = matchToken(re);
    if(!match.hasMatch()) return 0;
    return match.capturedRef(0).toUInt(nullptr, 2);
}

uint AscParser::parseDecimal()
{
    static const QRegularExpression re("[0-9]+");
    QRegularExpressionMatch match = matchToken(re);
    if(!match.hasMatch()) return 0;
    return match.capturedRef(0).toUInt(nullptr, 10);
}

QString AscParser::parseName()
{
    static const QRegularExpression re("[A-Za-z0-9$\\\\][A-Za-z0-9_/\\[\\].:$\\\\]*");
    QRegularExpressionMatch match = matchToken(re);
    if(!match.hasMatch()) return "";
    return match.captured(0);
}
