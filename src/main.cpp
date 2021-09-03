/**
 *  @file   main.cpp
 *  @brief  Daily Logger
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-09-03
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

typedef enum {
  OK,
  CONFIG_READ,
  CONFIG_WRITE,
  IO_READ,
  IO_WRITE,
  NO_QUERY,
  NOT_FOUND,
  STRUCTURE,
  UNKNOWN
} ERROR_CODE;

typedef string PREV_NEXT[2];

#define PREV 0
#define NEXT 1

ERROR_CODE readConfig(const char *file, string &config);
ERROR_CODE writeConfig(const char *file, string config);
int setConfig(string &config, string option, string value);
ERROR_CODE saveConfig(const char *file, string &config);

ERROR_CODE doRead(string ID);
ERROR_CODE doView(string ID);
ERROR_CODE doSearch(string ID);
ERROR_CODE doSave(string ID);
ERROR_CODE doSetup();

ERROR_CODE newEntry(string ID, string content);

string getID(void);
string ascID(string ID);
string readID(string str);

void openEntry(string ID, string content = "");
void viewEntry(string ID, string content, PREV_NEXT prev_next = NULL);

void errorMessage(string handle, ERROR_CODE code);
void header(void);
void footer(void);
void menu(string match = "");

void matchedHeader(string match);
void addMatched(string content, int at, string match, string ID = "");
void matchedFooter(int matched);
int find(string match, string str);

string decodeURL(const string URLencoded);
string toHTML(string noneHTML);

const string getvalue(const char *value, const string searchStr);
const string itostr(int i);
const string ftostr(float f, int signif);

string highlight(string content, string match);

string getOptions(const string directory);

string select(const string options, const string selected);

string dirstat(const string directory);
string filestat(const string file);
string filenew(const string file);

const char *entries = "<!--- BEGIN ENTRIES >";
const char *endEntries = "<!--- END ENTRIES >";

const char *entryID = "<!--- ENTRY ID = ";

const char *contentID = "<!--- CONTENT ID = ";
const char *endContent = "<!--- END CONTENT >";

string self;

string config;
string query;
string stream;
int main() {

  // self = string("http://") + getenv("HTTP_HOST") + getenv("SCRIPT_NAME");

  self = string("") + getenv("SCRIPT_NAME");

  getline(cin, stream);
  string action, ID, match;

  if (NULL != getenv("QUERY_STRING"))
    query = getenv("QUERY_STRING");
  action = getvalue("action", query);
  ID = getvalue("ID", query);

  match = decodeURL(getvalue("match", query));

  ERROR_CODE state = OK;

  if (getvalue("save", stream) == "true")
    state = saveConfig("bol.cfg", config);
  else {
    if (access("bol.cfg", F_OK) == 0)
      state = readConfig("bol.cfg", config);
    else
      action = "setup";
  }

  header();
  menu(match);
  if (state == OK) {
    if (action == "today")
      state = doRead(getID());
    else if (action == "edit")
      state = doRead(ID);
    else if (action == "view")
      state = doView(ID);
    else if (action == "search")
      state = doSearch(ID);
    else if (action == "save") {
      state = doSave(ID);
      if (state == OK)
        state = doView(ID);
    } else if (action == "setup")
      state = doSetup();
    else
      state = NO_QUERY;
  }

  if (state != OK)
    errorMessage(query, state);

  footer();

  return (OK);
}

ERROR_CODE doRead(string ID) {

  ifstream ifstr(getvalue("log", config).c_str(), ios::in);
  if (ifstr.fail())
    return (IO_READ);

  string entry;
  entry = entryID + ID + " >";

  string line = "";
  while (line.find(entry) == string::npos && ifstr.good())
    getline(ifstr, line);

  if (!ifstr.good())
    openEntry(ID);
  else {
    string content = "";
    entry = contentID + ID + " >";

    while (line.find(entry) == string::npos && ifstr.good())
      getline(ifstr, line);
    if (!ifstr.good())
      return (NOT_FOUND);

    getline(ifstr, line);
    while (line.find(endContent) == string::npos && ifstr.good()) {
      content += line + "\n";
      getline(ifstr, line);
    }

    if (!ifstr.good())
      return (STRUCTURE);

    openEntry(ID, content);
  }
  ifstr.close();

  return (OK);
}

ERROR_CODE doView(string ID = "") {
  ifstream ifstr(getvalue("log", config).c_str(), ios::in);
  if (ifstr.fail())
    return (IO_READ);

  string entry, line, content = "";
  if (ID.empty()) {
    while (ifstr.good()) {
      do {
        getline(ifstr, line);
        if (line.find(endEntries) != string::npos) {
          ifstr.close();
          return (OK);
        }
      } while (line.find(entryID) == string::npos && ifstr.good());
      if (!ifstr.good())
        return (STRUCTURE);

      ID = line.substr(line.find("=") + 2, 8);
      entry = contentID + ID + " >";

      do {
        getline(ifstr, line);
      } while (line.find(entry) == string::npos && ifstr.good());
      if (!ifstr.good())
        return (STRUCTURE);

      entry = endContent;
      content = "";
      getline(ifstr, line);

      do {
        content += line + "\n";
        getline(ifstr, line);
      } while (line.find(entry) == string::npos && ifstr.good());
      viewEntry(ID, content);
    }

    return (OK);
  }

  entry = contentID + ID + " >";
  PREV_NEXT prev_next = {"", ""};
  while (ifstr.good()) {
    if (line.find(contentID) != string::npos) {
      if (line.find(entry) != string::npos)
        break;
      prev_next[NEXT] = readID(line);
    }
    getline(ifstr, line);
  }
  if (!ifstr.good())
    return (NOT_FOUND);
  getline(ifstr, line);
  do {
    content += line + '\n';
    getline(ifstr, line);
  } while (line.find(endContent) == string::npos && ifstr.good());

  if (!ifstr.good())
    return (STRUCTURE);

  while (line.find(contentID) == string::npos && ifstr.good())
    getline(ifstr, line);
  if (ifstr.good())
    prev_next[PREV] = readID(line);

  viewEntry(ID, content, prev_next);
  return (OK);
}

ERROR_CODE doSearch(string ID = "") {
  if (!ID.empty()) {
    ifstream ifstr(getvalue("log", config).c_str(), ios::in);
    if (ifstr.fail())
      return (IO_READ);

    string match, line;
    match = decodeURL(getvalue("match", query));
    if (match.empty())
      return (OK);

    matchedHeader(match);

    int matched = 0;

    while (ifstr.good()) {
      while (line.find(entryID) == string::npos && ifstr.good()) {
        getline(ifstr, line);
        if (line.find(endEntries) != string::npos) {
          matchedFooter(matched);
          ifstr.close();
          return (OK);
        }
      }
      if (!ifstr.good())
        return (STRUCTURE);

      string entry;
      ID = line.substr(line.find("=") + 2, 8);
      entry = contentID + ID + " >";

      while (line.find(entry) == string::npos && ifstr.good())
        getline(ifstr, line);
      if (!ifstr.good())
        return (NOT_FOUND);

      int at = 0;
      bool newID = 1;
      while (line.find(endContent) == string::npos && ifstr.good()) {
        getline(ifstr, line);
        at++;
        if (find(match, line)) {
          matched++;
          if (newID) {
            newID = 0;
            addMatched(line, at, match, ID);
          } else
            addMatched(line, at, match);
        }
      }
      if (!ifstr.good())
        return (STRUCTURE);
    }
  }

  return (OK);
}

ERROR_CODE doSave(string ID = "") {
  ifstream ifstr(getvalue("log", config).c_str(), ios::in);
  if (ifstr.fail())
    return (IO_READ);

  ostringstream ostrstr;
  string line, id;
  while (ID != id) {
    do {
      getline(ifstr, line);
      ostrstr << line << endl;
    } while (line.find(contentID) == string::npos && ifstr.good());
    if (!ifstr.good()) {
      if (ID == getID()) {
        ifstr.close();
        return (newEntry(getID(), decodeURL(getvalue("content", stream))));
      }
      return (NOT_FOUND);
    }
    id = line.substr(line.find("=") + 2, 8);
  }

  ostrstr << decodeURL(getvalue("content", stream)) << endl;

  while (line.find(endContent) == string::npos && ifstr.good())
    getline(ifstr, line);
  if (!ifstr.good())
    return (STRUCTURE);

  do {
    ostrstr << line << endl;
    getline(ifstr, line);
  } while (ifstr.good());
  ifstr.close();

  ofstream ofstr(getvalue("log", config).c_str(), ios::out);
  if (ofstr.fail())
    return (IO_WRITE);

  ofstr << ostrstr.str();

  ofstr.close();

  return (OK);
}

ERROR_CODE newEntry(string ID, string content) {
  ifstream ifstr(getvalue("log", config).c_str(), ios::in);
  if (!ifstr.good())
    return (IO_READ);

  ostringstream ostrstr;
  string line = "";
  while (line.find(entries) == string::npos && ifstr.good()) {
    getline(ifstr, line);
    ostrstr << line << endl;
  }
  if (!ifstr.good())
    return (STRUCTURE);

  ostrstr << "  " << entryID << ID << " >" << endl
          << "    " << contentID << ID << " >" << endl
          << content << endl
          << endContent << endl
          << endl;

  while (ifstr.good()) {
    getline(ifstr, line);
    ostrstr << line << endl;
  }

  ifstr.close();

  ofstream ofstr(getvalue("log", config).c_str(), ios::out);
  if (!ofstr.good())
    return (IO_WRITE);

  ofstr << ostrstr.str();

  ofstr.close();

  return (OK);
}

string getID(void) {
  struct tm stm;
  time_t t;
  time(&t);
  stm = *localtime(&t);

  string ID;
  ID = (itostr(stm.tm_mday).length() == 2 ? "" : "0") + itostr(stm.tm_mday) +
       (itostr(stm.tm_mon + 1).length() == 2 ? "" : "0") +
       itostr(stm.tm_mon + 1) + itostr(stm.tm_year + 1900);

  return (ID);
}

string ascID(string ID) {

  int month;
  month = atoi(ID.substr(2, 2).c_str());

  string asc;
  if (ID.at(0) > '0')
    asc = ID.substr(0, 2) + " ";
  else
    asc = ID.substr(1, 1) + " ";

  switch (month) {
  case 1:
    asc += "January";
    break;
  case 2:
    asc += "February";
    break;
  case 3:
    asc += "March";
    break;
  case 4:
    asc += "April";
    break;
  case 5:
    asc += "May";
    break;
  case 6:
    asc += "June";
    break;
  case 7:
    asc += "July";
    break;
  case 8:
    asc += "August";
    break;
  case 9:
    asc += "September";
    break;
  case 10:
    asc += "October";
    break;
  case 11:
    asc += "November";
    break;
  case 12:
    asc += "December";
  };

  asc += " " + ID.substr(4, 4);

  return (asc);
}

string readID(string str) {
  return (str.substr(str.find_first_of("=") + 2, 8));
}

void viewEntry(string ID, string content, PREV_NEXT prev_next) {

  string match = decodeURL(getvalue("highlight", query));
  if (!match.empty())
    content = highlight(content, match);

  string previous = "", next = "";
  if (prev_next != NULL) {
    if (!prev_next[PREV].empty())
      previous = "       <span title=\"View previous (" +
                 ascID(prev_next[PREV]) + ")\"><a href=\"" + self +
                 "?action=view&ID=" + prev_next[PREV] +
                 "\" onmouseover=\"window.status='Previous entry';return "
                 "true\" onmouseout=\"window.status=' '\"><img src=\"" +
                 getvalue("base", config) +
                 "images/previous.gif\" alt=\"Previous entry\" border=\"0\" "
                 "/></a></span>";
    else
      previous = string("       <img src=\"") + getvalue("base", config) +
                 "images/previous_disabled.gif\"\" alt=\"disabled\">";

    if (!prev_next[NEXT].empty())
      next = "       <span title=\"View next (" + ascID(prev_next[NEXT]) +
             ")\"><a href=\"" + self + "?action=view&ID=" + prev_next[NEXT] +
             "\" onmouseover=\"window.status='Next entry';return true\" "
             "onmouseout=\"window.status=' '\"><img src=\"" +
             getvalue("base", config) +
             "images/next.gif\" alt=\"Next entry\" border=\"0\" /></a></span>";
    else
      next = string("       <img src=\"") + getvalue("base", config) +
             "images/next_disabled.gif\"\" alt=\"disabled\">";
  }

  cout << "<br />" << endl
       << "<table align=\"center\" width=\"600\" rules=\"none\" "
          "cellspacing=\"1\" cellpadding=\"3\" class=\"entry\">"
       << endl
       << "  <tr>" << endl
       << "    <td width=\"50%\" align=\"left\">" << endl
       << previous << next << "    </td>" << endl
       << "    <td align=\"right\" class=\"date\">" << endl
       << ascID(ID) << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td colspan=\"2\" valign=\"top\" class=\"content\">" << endl
       << "      <br />" << endl
       << toHTML(content) << endl
       << endl
       << "      <br />" << endl
       << "    </td>" << endl
       << "  </tr>" << endl

       << "  <tr>" << endl
       << "    <td colspan=\"2\" align=\"right\">" << endl
       << "      <span title=\"View " << ascID(ID) << "\"><a href=\"" << self
       << "?action=view&ID=" << ID
       << "\" onmouseover=\"window.status='View alone';return true\" "
          "onmouseout=\"window.status=' '\">&nbsp;View&nbsp;</a></span>"
       << endl
       << "      <span title=\"Edit " << ascID(ID) << "\"><a href=\"" << self
       << "?action=edit&ID=" << ID
       << "\" onmouseover=\"window.status='Edit entry';return true\" "
          "onmouseout=\"window.status=' '\">&nbsp;Edit&nbsp;</a></span>"
       << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "</table>" << endl
       << endl
       << "<br />" << endl;
}

void openEntry(string ID, string content) {

  cout << "<br />" << endl
       << "<form name=\"form\" action=\"" << self << "?action=save&ID=" << ID
       << "\" method=\"POST\">" << endl
       << "<table align=\"center\" width=\"600\" rules=\"none\" "
          "cellspacing=\"0\" cellpadding=\"3\" class=\"entry\">"
       << endl
       << "  <tr>" << endl
       << "    <td align=\"right\" class=\"date\">" << endl
       << ascID(ID) << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td align=\"center\" class=\"content\">" << endl
       << "      <br />" << endl
       << "      <textarea name=\"content\" class=\"wordprocessor\">"
       << content.substr(0, content.length() - 1) << "</textarea><br />" << endl
       << endl
       << "      <br />" << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td align=\"right\">" << endl
       << "       <a href=\"JavaScript:document.form.submit()\" "
          "onmouseover=\"window.status='Save Entry';return true\" "
          "onmouseout=\"window.status=' '\">&nbsp;Save&nbsp;</a>"
       << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "</table>" << endl
       << endl
       << "</form>";
}

void matchedHeader(string match) {

  cout << "<br />" << endl
       << "<table align=\"center\" width=\"600\" rules=\"none\" "
          "cellspacing=\"0\" cellpadding=\"3\" class=\"menu\" >"
       << endl
       << "  <tr>" << endl
       << "    <td colspan=\"3\">" << endl
       << "      <font size=\"4\"><b>Results for <i>" << match
       << "</i></b></font><br />" << endl
       << "      <br />" << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "    <td align=\"left\" width=\"210\">" << endl
       << "      <i>Date</i>" << endl
       << "    </td>" << endl
       << "    <td align=\"left\" width=\"360\">" << endl
       << "       <i>Content</i>" << endl
       << "    </td>" << endl
       << "    <td align=\"right\" width=\"30\">" << endl
       << "       <i>#</i>" << endl
       << "    </td>" << endl
       << "  </tr>" << endl;
}

void addMatched(string content, int at, string match, string ID) {

  cout << "  <tr>" << endl
       << "    <td align=\"left\" valign=\"top\" width=\"210\">" << endl;

  if (ID.empty())
    cout << "      &nbsp;" << endl;
  else
    cout << "      <a href=\"" << self << "?action=view&ID=" << ID
         << "&highlight=" << match
         << "\" onmouseover=\"window.status='View';return true\" "
            "onmouseout=\"window.status=' '\">"
         << ascID(ID) << "</a>" << endl;

  cout << "    </td>" << endl
       << "    <td align=\"left\" width=\"360\">" << endl
       << content << endl
       << "    </td>" << endl
       << "    <td align=\"right\" width=\"30\">" << endl
       << "      <i><b>" << at << "</b></i>" << endl
       << "    </td>" << endl
       << "  </tr>" << endl;
}

void matchedFooter(int matched) {

  cout << "  <tr>" << endl
       << "    <td align=\"right\" COLSPAN=\"3\">" << endl
       << "      <font size=\"5\"><b>" << matched << " matches</b></font>"
       << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "</table>" << endl
       << endl;
}

void header(void) {
  cout << "Content-type: text/html; charset=iso-8859-1" << endl
       << endl
       << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
          "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
       << endl
       << endl
       << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl
       << endl
       << "  <head>" << endl
       << endl
       << "    <link rel=\"SHORTCUT ICON\" href=\"" << getvalue("base", config)
       << "/bol.ico\" />" << endl
       << endl
       << "    <link rel=\"stylesheet\" href=\""
       << getvalue("base", config) << getvalue("plugin", config)
       << getvalue("scheme", config) << ".css\" type=\"text/css\" />" << endl
       << endl
       << "    <title>Boersma online Logbook - BoL</title>" << endl
       << endl
       << "    <meta name=\"description\" content=\"Homepage Christiaan "
          "Boersma\" />"
       << endl
       << "    <meta name=\"keywords\" content=\"Christiaan, Boersma, RuG\" />"
       << endl
       << endl
       << "    <meta name=\"resource-type\" content=\"document\" />" << endl
       << "    <meta name=\"robots\" content=\"noimageclick\" />" << endl
       << "     <meta name=\"pragma\" content=\"no-cache\" />" << endl
       << endl
       << "</head>" << endl
       << endl
       << "<body class=\"main\">" << endl
       << endl;
}

void footer(void) {

  struct tm stm;
  time_t t;
  time(&t);
  stm = *localtime(&t);

  char year[5];

  strftime(year, 5, "%Y", &stm);

  cout << "<br />" << endl
       << "<table align=\"center\" width=\"600\" rules=\"none\" "
          "cellspacing=\"1\" cellpadding=\"3\" class=\"menu\">"
       << endl
       << "  <tr>" << endl
       << "    <td align=\"left\">" << endl
       << "version 2.1" << endl
       << "    </td>" << endl
       << "    <td align=\"right\">" << endl
       << "&#169; Christiaan Boersma (2004/" << year << ')' << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "</table>" << endl
       << "</body>" << endl
       << endl
       << "</html>" << endl;
}

void menu(string match) {

  cout << "<br />" << endl
       << "<br />" << endl
       << "<form name=\"search\" action=\"" << self
       << "?action=search&ID=000000\" method=\"get\">" << endl
       << "<table align=\"center\" width=\"600\" rules=\"none\" "
          "cellspacing=\"0\" cellpadding=\"10\" class=\"menu\" >"
       << endl
       << "  <tr>" << endl
       << "    <td valign=\"bottom\">" << endl
       << "      <span title=\"Edit today\"><a href=\"" << self
       << "?action=today\" onmouseover=\"window.status='Today';return true\" "
          "onmouseout=\"window.status=' "
          "'\">&nbsp;Today&nbsp;</a></span>&nbsp; "
          "&nbsp;<span title=\"View all entries\"><a href=\""
       << self
       << "?action=view\" onmouseover=\"window.status='View Entries';return "
          "true\" onmouseout=\"window.status=' '\">&nbsp;View "
          "All&nbsp;</a></span> &nbsp; &nbsp;  &nbsp; &nbsp; <span "
          "title=\"BoL "
          "Setup\"><a href=\""
       << self
       << "?action=setup\" onmouseover=\"window.status='BoL "
          "Configuration';return true\" onmouseout=\"window.status=' "
          "'\">&nbsp;Setup&nbsp;</a></span>"
       << endl
       << "    </td>" << endl
       << "    <td align=\"right\" valign=\"bottom\">" << endl
       << "        <input type=\"hidden\" name=\"action\" "
          "value=\"search\"/><span title=\"Search, hot-key Alt + S, Ctrl + S "
          "(Apple)\"><input type=\"hidden\" name=\"ID\" "
          "value=\"000000\"/><input class=\"search\" type=\"text\" "
          "name=\"match\" size=\"30\" value=\""
       << match
       << "\" accesskey=\"s\"/> &nbsp; <a "
          "href=\"JavaScript:document.search.submit()\" "
          "onmouseover=\"window.status='Search';return true\" "
          "onmouseout=\"window.status=' '\">&nbsp;Search&nbsp;</a></span>"
       << endl
       << "   </td>" << endl
       << "  </tr>" << endl
       << "</table>" << endl
       << "</form>" << endl;
}

void errorMessage(string handle, ERROR_CODE code) {

  string message;
  switch (code) {
  case CONFIG_READ:
    message = "Unable to read from config file";
    break;
  case CONFIG_WRITE:
    message = "Unable to write to config file";
    break;
  case IO_READ:
    message = "Unable to read from log file";
    break;
  case IO_WRITE:
    message = "Unable to write to log file";
    break;
  case NO_QUERY:
    message = "Requested action unknown";
    break;
  case NOT_FOUND:
    message = "Entry not found";
    break;
  case STRUCTURE:
    message = "Structure fault in log file";
    break;
  default:
    message = "Unknown fault";
  };

  cout << "<br />" << endl
       << "<table align=\"center\" width=\"600\" rules=\"none\" "
          "cellspacing=\"0\" cellpadding=\"3\" class=\"menu\">"
       << endl
       << "  <tr>" << endl
       << "    <td>" << endl
       << "      <H1>An error occured!</H1>" << endl
       << "      <br />" << endl
       << "      Cannot execute: <i><b>" << handle << "</i></b><br />" << endl
       << "      Error code    : <i><b>" << code << "</i></b><br />" << endl
       << "      &nbsp;&nbsp; " << message << "<br />" << endl
       << "      <br />" << endl
       << "      If the error persists contact the administrator:<br />" << endl
       << "      <br />" << endl
       << "      <a href=\"mailto:" << getvalue("administrator", config)
       << "\" onmouseover=\"window.status='Contact the Administrator';return "
          "true\" onmouseout=\"window.status=' '\">"
       << getvalue("administrator", config) << "</a><br />" << endl
       << "      <br />" << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "</table>" << endl;
}

int find(string match, string str) {
  for (size_t i = 0; i < str.length(); i++) {
    if (str.at(i) == toupper(match.at(0)) ||
        str.at(i) == tolower(match.at(0))) {
      if ((str.length() - i) < match.length())
        return (0);
      else {
        size_t j = 0;
        while (str.at(i + j) == toupper(match.at(j)) ||
               str.at(i + j) == tolower(match.at(j))) {
          if (j == match.length() - 1)
            return (1);
          j++;
        }
      }
    }
  }
  return (0);
}

const string getvalue(const char *value, const string searchStr) {
  string::size_type begin = searchStr.find(value),
                    end = searchStr.find_first_of("&", begin);

  if (begin == string::npos)
    return ("");

  if (end == string::npos)
    end = searchStr.length();

  string paired = searchStr.substr(begin, end - begin);
  begin = paired.find("=");
  return (paired.substr(begin + 1));
}

const string itostr(int i) {
  ostringstream ostrstr;
  ostrstr << i;
  return (ostrstr.str());
}

const string ftostr(float f, int signif) {
  ostringstream ostrstr;
  ostrstr.setf(ios::fixed);
  ostrstr << setprecision(signif) << f;
  return (ostrstr.str());
}

string decodeURL(const string URLencoded) {

  string URLdecoded(URLencoded);
  for (string::size_type idx = 0; idx < URLdecoded.length(); idx++) {
    if (URLdecoded.at(idx) == '+')
      URLdecoded.replace(idx, 1, " ");
    else if (URLdecoded.at(idx) == '%')
      URLdecoded.replace(idx, 3, 1,
                         static_cast<char>(strtol(
                             URLdecoded.substr(idx + 1, 2).c_str(), NULL, 16)));
  }

  return (URLdecoded);
}

string toHTML(string noneHTML) {
  for (size_t idx = 0; idx < noneHTML.length(); idx++) {
    if (noneHTML[idx] == '\n') {

      if (noneHTML[idx - 1] == '\r')
        noneHTML.erase(--idx, 1);

      noneHTML.replace(idx, 1, "<br />\n");
      idx = idx + 6;
    } else if (noneHTML[idx] == ' ') {
      if (noneHTML[idx + 1] == ' ') {
        noneHTML.replace(idx + 1, 1, "&nbsp;");
        idx = idx + 6;
      }
    }
  }

  return (noneHTML);
}

string highlight(string content, string match) {
  string workString = "";
  int contentLen = content.length(), matchLen = match.length();

  for (int pos = 0; pos < contentLen; pos++) {
    if (content.at(pos) == '<') {
      while (content.at(pos) != '>') {
        workString += content.at(pos);
        if (++pos > contentLen)
          return (workString);
      }
      workString += content.at(pos);
    } else {
      int seek = 0, postmp = pos;
      while (tolower(content.at(postmp)) == tolower(match.at(seek))) {
        if ((++postmp > contentLen - 1) || (++seek > matchLen - 1))
          break;
      }
      if (seek == matchLen) {
        workString += "<span class=\"highlight\" title=\"highlighted\">";
        for (; pos < postmp; pos++)
          workString += content.at(pos);
        workString += "</span>";
      }
      workString += content.at(pos);
    }
  }
  return (workString);
}

ERROR_CODE readConfig(const char *file, string &config) {
  config = "";

  ifstream ifstr(file, ios::in);
  if (ifstr.fail())
    return (CONFIG_READ);

  char character;
  while (ifstr.get(character).good()) {
    if (character == '#') {
      while (ifstr.get(character).good()) {
        if (character == '\n')
          break;
      }
    } else if (character == '$') {
      if (!config.empty())
        config += '&';
      while (ifstr.get(character).good()) {
        if (character == '\n')
          break;
        if (character == '\\') {
          while (ifstr.get(character).good())
            if (character == '\n')
              break;
        } else if (character != ' ' && character != '\"')
          config += character;
      }
    }
  }
  ifstr.close();
  return (OK);
}

ERROR_CODE writeConfig(const char *file, string config) {
  ofstream ofstr(file, ios::out);
  if (ofstr.fail())
    return (CONFIG_WRITE);

  ofstr << "#" << endl
        << "# Automatic generated configuration file for BoL" << endl
        << "#" << endl
        << "# valid variables are $log, $base, $administrator, $schemes and "
           "$scheme"
        << endl
        << "#" << endl
        << endl;

  int pos = 0, configLen = config.length();
  while (pos < configLen) {
    ofstr << '$';
    while (config.at(pos) != '=')
      ofstr << config.at(pos++);
    ofstr << " " << config.at(pos++) << " \"";
    while (pos < configLen) {
      if (config.at(pos) == '&') {
        ++pos;
        break;
      }
      ofstr << config.at(pos++);
    }
    ofstr << "\"" << endl;
  }
  ofstr.close();

  return (OK);
}

int setConfig(string &config, string option, string value) {
  string::size_type start = config.find(option),
                    end = config.find_first_of("&", start);

  if (end == string::npos)
    end = config.length();

  if (start == string::npos) {
    if (!config.empty())
      config += '&';
    start = config.length();
  } else
    config.erase(start, end - start);

  config.insert(start, option + "=" + value);

  return (0);
}

ERROR_CODE saveConfig(const char *file, string &config) {
  setConfig(config, "log", decodeURL(getvalue("log", stream)));
  setConfig(config, "base", decodeURL(getvalue("base", stream)));
  setConfig(config, "administrator",
            decodeURL(getvalue("administrator", stream)));
  setConfig(config, "plugin", decodeURL(getvalue("plugin", stream)));
  setConfig(config, "scheme", decodeURL(getvalue("scheme", stream)));
  return (writeConfig(file, config));
}

ERROR_CODE doSetup() {

  cout << "<br />" << endl
       << "<form name=\"setup\" action=\"" << self
       << "?action=setup\" method=\"POST\">" << endl
       << "<input type=\"hidden\" name=\"save\" value=\"true\" />" << endl
       << "<table align=\"center\" width=\"600\" rules=\"none\" "
          "cellspacing=\"0\" cellpadding=\"3\" class=\"menu\">"
       << endl
       << "  <tr>" << endl
       << "    <td colspan=\"2\">" << endl
       << "      <h1>BoL Setup</h1>" << endl
       << "      <h2>General</h2>" << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td width=\"20%\">" << endl
       << "      Log file " << endl
       << "    </td>" << endl
       << "    <td>" << endl
       << "      <input class=\"setup\" type=\"text\" name=\"log\" value=\""
       << getvalue("log", config) << "\" /> "
       << filenew(getvalue("log", config)) << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td>" << endl
       << "      Base URL " << endl
       << "    </td>" << endl
       << "    <td>" << endl
       << "      <input class=\"setup\" type=\"text\" name=\"base\" value=\""
       << getvalue("base", config) << "\" />" << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td>" << endl
       << "      Plugin DIR " << endl
       << "    </td>" << endl
       << "    <td>" << endl
       << "      <input class=\"setup\" type=\"text\" name=\"plugin\" value=\""
       << getvalue("plugin", config) << "\" /> "
       << dirstat(getvalue("plugin", config)) << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td>" << endl
       << "      Administrator" << endl
       << "    </td>" << endl
       << "    <td>" << endl
       << "      <input class=\"setup\" type=\"text\" name=\"administrator\" "
          "value=\""
       << getvalue("administrator", config) << "\" />" << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td colspan=\"2\">" << endl
       << "      <h2>Look &amp; Feel</h2>" << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td>" << endl
       << "      Theme" << endl
       << "    </td>" << endl
       << "    <td>" << endl
       << select(getOptions(decodeURL(getvalue("plugin", config))),
                 decodeURL(getvalue("scheme", config)))
       << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "  <tr>" << endl
       << "    <td colspan=\"2\" align=\"right\">" << endl
       << "      <span title=\"Save setup\"><a "
          "href=\"javascript:document.setup.submit();\" "
          "onmouseover=\"window.status='Save setup';return true\" "
          "onmouseout=\"window.status=' '\">&nbsp;Save&nbsp;</a></span> <span "
          "title=\"Undo changes\"><a "
          "href=\"javascript:document.setup.reset();\" "
          "onmouseover=\"window.status='Undo changes';return true\" "
          "onmouseout=\"window.status=' '\">&nbsp;Undo&nbsp;</a></span>"
       << endl
       << "    </td>" << endl
       << "  </tr>" << endl
       << "</table>" << endl
       << "</form>" << endl;

  return (OK);
}

string filestat(const string file) {
  if (!file.empty()) {
    if (access(file.c_str(), R_OK) != 0)
      return (" <font color=\"#ff0000\">File not found!</font>");
    else {
      struct stat f_stat;
      stat(file.c_str(), &f_stat);
      if (f_stat.st_size < 1e3)
        return (" size " + itostr(f_stat.st_size) + " bytes");
      else
        return (" size " + ftostr(f_stat.st_size / 1e3, 1) + " KB");
    }
  }
  return ("");
}

string filenew(const string file) {

  if (!file.empty()) {
    if (access(file.c_str(), W_OK) != 0) {
      ofstream newfile(file.c_str(), ios::out);

      newfile << entries << endl << endEntries;

      newfile.close();
      return (" <b>new</b>");
    } else {
      struct stat f_stat;
      stat(file.c_str(), &f_stat);
      if (f_stat.st_size < 1e3)
        return (" size " + itostr(f_stat.st_size) + " bytes");
      else
        return (" size " + ftostr(f_stat.st_size / 1e3, 1) + " kb");
    }
  }
  return ("");
}

string dirstat(const string directory) {
  if (!directory.empty()) {
    if (access(directory.c_str(), F_OK) != 0)
      return (" <font color=\"#ff0000\">Directory does not exist!</font>");
  }
  return ("");
}

string select(const string options, const string selected) {
  string workString = "      <select class=\"select\" name=\"scheme\">", option;

  string::size_type start = 0, end;

  while (start < options.length()) {
    end = options.find_first_of("&", start);
    if (end == string::npos)
      end = options.length();

    option = options.substr(start, end - start);
    workString += "\n        <option value=\"" + option + "\"";
    if (selected == option)
      workString += " selected=\"selected\"";
    workString += ">" + option + "</option>";
    start = end + 1;
  }

  workString += "\n      </select>";

  return (workString);
}

string getOptions(const string directory) {
  struct dirent **listing;
  int n_files;
  if ((n_files = scandir(directory.c_str(), &listing, NULL, alphasort)) < 0)
    return ("");

  string files = "";
  for (int file_nr = 0; file_nr < n_files; file_nr++) {
    if (strlen(listing[file_nr]->d_name) > 4) {
      if (strcmp(listing[file_nr]->d_name + strlen(listing[file_nr]->d_name) -
                     4,
                 ".css") == 0) {
        if (!files.empty())
          files += '&';
        files += string(listing[file_nr]->d_name)
                     .substr(0, strlen(listing[file_nr]->d_name) - 4);
      }
    }
  }

  return (files);
}
