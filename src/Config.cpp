#include "Config.h"
#include <stdlib.h>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/stat.h>
#endif

Config::Config(const std::string& path)
	: path(path)
{
	std::ifstream in(path.c_str());
	std::string line;
	while (std::getline(in, line))
	{
		size_t equalPos = line.find('=');
		if (equalPos != std::string::npos)
		{
			values[line.substr(0, equalPos)] = line.substr(equalPos + 1);
		}
	}
}

std::string Config::defaultPath()
{
	std::string directory;
#ifdef _WIN32
	char appData[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appData)))
	{
		directory = std::string(appData) + "\\XGSEmu";
		CreateDirectoryA(directory.c_str(), NULL);
	}
	else
	{
		directory = ".";
	}
	return directory + "\\config.ini";
#else
	const char* home = getenv("HOME");
	if (home != NULL)
	{
		// ~/.config may not be there yet, mkdir makes one directory only
		std::string config = std::string(home) + "/.config";
		directory = config + "/xgsemu";
		mkdir(config.c_str(), 0755);
		mkdir(directory.c_str(), 0755);
	}
	else
	{
		directory = ".";
	}
	return directory + "/config.ini";
#endif
}

std::string Config::get(const std::string& key, const std::string& defaultValue) const
{
	std::map<std::string, std::string>::const_iterator found = values.find(key);
	return found == values.end() ? defaultValue : found->second;
}

void Config::set(const std::string& key, const std::string& value)
{
	values[key] = value;
	std::ofstream out(path.c_str());
	for (std::map<std::string, std::string>::const_iterator i = values.begin(); i != values.end(); ++i)
	{
		out << i->first << "=" << i->second << "\n";
	}
}
