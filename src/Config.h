#ifndef CONFIG_H_
#define CONFIG_H_

#include <map>
#include <string>

/**
 * A few settings that stay between two runs: "key=value" lines in a text file.
 * A file that cannot be read or written is no error, the settings are just not remembered.
 */
class Config
{
public:
	/** The settings in this file; they are read now. */
	explicit Config(const std::string& path);

	/** The file in the configuration directory of the user (it is created if needed). */
	static std::string defaultPath();

	std::string get(const std::string& key, const std::string& defaultValue = "") const;

	/** Sets a value and writes the file. */
	void set(const std::string& key, const std::string& value);

private:
	std::string path;
	std::map<std::string, std::string> values;
};

#endif /*CONFIG_H_*/
