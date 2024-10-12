
#include "std.h"
#include <cctype>
#include "toker.h"
#include "ex.h"
#include "parser.h"

int Toker::chars_toked;

static map<string, int> alphaTokes, lowerTokes;

static void makeKeywords() {
	static bool made;
	if (made) return;

	alphaTokes["Dim"] = DIM;
	alphaTokes["Goto"] = GOTO;
	alphaTokes["Gosub"] = GOSUB;
	alphaTokes["Return"] = RETURN;
	alphaTokes["Exit"] = EXIT;
	alphaTokes["If"] = IF;
	alphaTokes["Then"] = THEN;
	alphaTokes["Else"] = ELSE;
	alphaTokes["EndIf"] = ENDIF;
	alphaTokes["End If"] = ENDIF;
	alphaTokes["ElseIf"] = ELSEIF;
	alphaTokes["Else If"] = ELSEIF;
	alphaTokes["While"] = WHILE;
	alphaTokes["Wend"] = WEND;
	alphaTokes["For"] = FOR;
	alphaTokes["To"] = TO;
	alphaTokes["Step"] = STEP;
	alphaTokes["Next"] = NEXT;
	alphaTokes["Function"] = FUNCTION;
	alphaTokes["EndFunction"] = ENDFUNCTION;
	alphaTokes["End Function"] = ENDFUNCTION;
	alphaTokes["Type"] = TYPE;
	alphaTokes["EndType"] = ENDTYPE;
	alphaTokes["End Type"] = ENDTYPE;
	alphaTokes["Each"] = EACH;
	alphaTokes["Local"] = LOCAL;
	alphaTokes["Global"] = GLOBAL;
	alphaTokes["Field"] = FIELD;
	alphaTokes["Const"] = BBCONST;
	alphaTokes["Select"] = SELECT;
	alphaTokes["Case"] = CASE;
	alphaTokes["Default"] = DEFAULT;
	alphaTokes["EndSelect"] = ENDSELECT;
	alphaTokes["End Select"] = ENDSELECT;
	alphaTokes["Repeat"] = REPEAT;
	alphaTokes["Until"] = UNTIL;
	alphaTokes["Forever"] = FOREVER;
	alphaTokes["Data"] = DATA;
	alphaTokes["Read"] = READ;
	alphaTokes["Restore"] = RESTORE;
	alphaTokes["Abs"] = ABS;
	alphaTokes["Sgn"] = SGN;
	alphaTokes["Mod"] = MOD;
	alphaTokes["Pi"] = PI;
	alphaTokes["True"] = BBTRUE;
	alphaTokes["False"] = BBFALSE;
	alphaTokes["Int"] = BBINT;
	alphaTokes["Float"] = BBFLOAT;
	alphaTokes["Str"] = BBSTR;
	alphaTokes["Include"] = INCLUDE;
	alphaTokes["Dialect"] = DIALECT;

	alphaTokes["New"] = BBNEW;
	alphaTokes["Delete"] = BBDELETE;
	alphaTokes["First"] = FIRST;
	alphaTokes["Last"] = LAST;
	alphaTokes["Insert"] = INSERT;
	alphaTokes["Before"] = BEFORE;
	alphaTokes["After"] = AFTER;
	alphaTokes["Null"] = BBNULL;
	alphaTokes["Object"] = OBJECT;
	alphaTokes["Handle"] = BBHANDLE;

	alphaTokes["And"] = AND;
	alphaTokes["Or"] = OR;
	alphaTokes["Xor"] = XOR;
	alphaTokes["Not"] = NOT;
	alphaTokes["Shl"] = SHL;
	alphaTokes["Shr"] = SHR;
	alphaTokes["Sar"] = SAR;

	map<string, int>::const_iterator it;
	for (it = alphaTokes.begin(); it != alphaTokes.end(); ++it) {
		lowerTokes[tolower(it->first)] = it->second;
	}
	made = true;
}

Toker::Toker(istream &in) : in(in), curr_row(-1), rem_nest(0) {
	makeKeywords();
	nextline();
}

map<string, int> &Toker::getKeywords() {
	makeKeywords();
	return alphaTokes;
}

int Toker::pos() {
	return ((curr_row) << 16) | (tokes[curr_toke].from);
}

int Toker::curr() {
	return tokes[curr_toke].n;
}

string Toker::text() {
	int from = tokes[curr_toke].from, to = tokes[curr_toke].to;
	return line.substr(from, to - from);
}

int Toker::lookAhead(int n) {
	return tokes[curr_toke + n].n;
}

void Toker::nextline() {
	++curr_row;
	curr_toke = 0;
	tokes.clear();
	if (in.eof()) {
		line.resize(1);
		line[0] = EOF;
		tokes.push_back(Toke(EOF, 0, 1));
		return;
	}

	getline(in, line);
	line += '\n';
	chars_toked += line.size();

	int k = 0;
	for (; line[k] != '\n' && isspace(line[k]); ++k) {}
	if (line[k] == '/' && line[k + 1] == '*') {
		++rem_nest;
		k += 2;
	} else if (line[k] == '*' && line[k + 1] == '/') {
		--rem_nest;
		k += 2;
	}
	if (rem_nest) {
		for (; line[k] != '\n'; ++k);
		tokes.push_back(Toke('\n', k, k + 1));
		return;
	}

	while (k < line.size()) {
		int c = line[k], from = k;
		int n = line[k + 1];
		if (c == '\n') {
			tokes.push_back(Toke(c, from, ++k));
			continue;
		}
		if (isspace(c)) {
			++k;
			continue;
		}
		if (c == ';' || (c == '/' && n == '/')) {
			for (++k; line[k] != '\n'; ++k) {}
			continue;
		}
		if (c == '.' && isdigit(line[k + 1])) {
			for (k += 2; isdigit(line[k]); ++k) {}
			tokes.push_back(Toke(FLOATCONST, from, k));
			continue;
		}
		if (isdigit(c)) {
			for (++k; isdigit(line[k]); ++k) {}
			if (line[k] == '.') {
				for (++k; isdigit(line[k]); ++k) {}
				tokes.push_back(Toke(FLOATCONST, from, k));
				continue;
			}
			tokes.push_back(Toke(INTCONST, from, k));
			continue;
		}
		if (c == '%' && (line[k + 1] == '0' || line[k + 1] == '1')) {
			for (k += 2; line[k] == '0' || line[k] == '1'; ++k) {}
			tokes.push_back(Toke(BINCONST, from, k));
			continue;
		}
		if (c == '$' && isxdigit(line[k + 1])) {
			for (k += 2; isxdigit(line[k]); ++k) {}
			tokes.push_back(Toke(HEXCONST, from, k));
			continue;
		}
		if (isalpha(c)) {
			for (++k; isalnum(line[k]) || line[k] == '_'; ++k) {}

			string ident = tolower(line.substr(from, k - from));

			if (line[k] == ' ' && isalpha(line[k + 1])) {
				int t = k;
				for (t += 2; isalnum(line[t]) || line[t] == '_'; ++t) {}
				string s = tolower(line.substr(from, t - from));
				if (lowerTokes.find(s) != lowerTokes.end()) {
					k = t;
					ident = s;
				}
			}

			map<string, int>::iterator it = lowerTokes.find(ident);

			if (it == lowerTokes.end()) {
				for (int n = from; n < k; ++n) line[n] = tolower(line[n]);
				tokes.push_back(Toke(IDENT, from, k));
				continue;
			}

			tokes.push_back(Toke(it->second, from, k));
			continue;
		}
		if (c == '\"') {
			for (++k; line[k] != '\"' && line[k] != '\n'; ++k) {}
			if (line[k] == '\"') ++k;
			tokes.push_back(Toke(STRINGCONST, from, k));
			continue;
		}
		if ((c == '<' && n == '>') || (c == '>' && n == '<')) {
			tokes.push_back(Toke(NE, from, k += 2));
			continue;
		}
		if ((c == '<' && n == '=') || (c == '=' && n == '<')) {
			tokes.push_back(Toke(LE, from, k += 2));
			continue;
		}
		if ((c == '>' && n == '=') || (c == '=' && n == '>')) {
			tokes.push_back(Toke(GE, from, k += 2));
			continue;
		}
		tokes.push_back(Toke(c, from, ++k));
	}
	if (!tokes.size()) exit(0);
}

// have to do this because the toker is an introvert and refuses to initiate conversation with the parser,
// so the parser has to be the one to initiate the conversation
//
// NOTE: currently, this code is depricated because we switched to 1.118's multiline code comment parser
string Toker::checkFailure()
{
	string retType = "";
	switch (failureType)
	{
	case 0:
		retType = "If you're seeing this, you somehow made failureType greater than 0 while still being 0.";
		break;
	default:
		retType = "Undefined Toker Failure!";
		break;
	}
	return retType;
}

int Toker::next() {
	if (++curr_toke == tokes.size()) nextline();
	return curr();
}
