/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *  You can also contact with mail(gangx.li@intel.com)
 */
#include <fstream>
#include <string>

using namespace std;

typedef enum _LOG_LEVEL
{
    LOG_LEVEL_INVALID = -1,
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} LOG_LEVEL;

typedef enum _LOG_MODE
{
    LOG_STDOUT = 0,
    LOG_FILE = 1
} LOG_MODE;

class Logger {

public:
    Logger(){};
    virtual ~Logger(){};

    static int SetLogLevel(const char* plevel);
    static int SetLogMode(const char* log_mode);
    static int SetLogSize(const char* log_size);

    static void LogOut(const int log_level, const char* format, ...);

private:
    Logger(const Logger&);
    void operator=(const Logger&);

    static void Init();
    static void PrepareUploadLog();

};

#define ENABLE_ENTRY_LOGGER 0
class EntryLogger {

public:
    EntryLogger(const char *pname): _name(pname) {
#if ENABLE_ENTRY_LOGGER
        if (!_name.empty())
            Logger::LogOut(LOG_LEVEL_ERROR, "%s, bgn", _name.c_str());
#endif
    }

    virtual ~EntryLogger() {
#if ENABLE_ENTRY_LOGGER
        if (!_name.empty())
            Logger::LogOut(LOG_LEVEL_ERROR, "%s, end", _name.c_str());
#endif
    }

private:
    const std::string _name;
};

