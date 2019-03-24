#include <sstream>
#include "arguments.hpp"

static bool endsWith(const string& text, const string& ending)
{
	if (text.length() >= ending.length())
	{
		return (0 == text.compare (text.length() - ending.length(), ending.length(), ending));
	}
	return false;
}

static vector<string> getValueArguments(const string& val)
{
	string value = val;
	replaceAll(value, "(", "");
	replaceAll(value, ")", "");
	replaceAll(value, " ", "");
	vector<string> result;
	string build;
	for(auto&& c : value)
	{
		if(c != ',')
		{
			char str[2] = {c, '\0'};
			build += string(str);
		}
		else if(!build.empty())
		{
			result.push_back(build);
			build.clear();
		}
		else
		{
			result.push_back("");
		}
	}
	if(!build.empty())
	{
		result.push_back(build);
	}
	return result;
}

Arguments::Arguments(int argc, char **argv)
{
	string argKey;
	for(int i=0; i<argc; i++)
	{
		string arg = argv[i];
		if(arg.length() > 0 && arg[0] == '-')
		{
			string val = arg.substr(1);
			if(argumentWithoutValue(val))
			{
				m_args[val] = val;
			}
			else
			{
				argKey = val;
			}
		}
		else if(!argKey.empty())
		{
			m_args[argKey] = arg;
		}
		else
		{
			m_args[arg] = arg;
		}
	}
}

string Arguments::getArgument(const string& key, const string& defaultValue) const
{
	if(hasArgument(key))
	{
		return m_args.at(key);
	}
	return defaultValue;
}

ArgumentResult<string> Arguments::getArgument(const string& key) const
{
	if(hasArgument(key))
	{
		return ArgumentResult<string>(m_args.at(key), "");
	}
	return ArgumentResult<string>("", getOptionNotFoundErrorMsg(key));
}

void Arguments::addArgument(const string& key)
{
	m_args[key] = key;	
}
void Arguments::addArgument(const string& key, const string& value)
{
	m_args[key] = value;
}

void Arguments::clear()
{
	m_args.clear();
}

bool Arguments::hasArgument(const string& key) const
{
	return m_args.count(key) > 0;
}

ArgumentResult<string> Arguments::getArgument_InputPath() const
{
	return getArgument("i");
}

ArgumentResult<string> Arguments::getArgument_OutputPath() const
{
	return getArgument("o");
}

ArgumentResult<Setting> Arguments::getArgument_Setting() const
{
	string v = getArgument("s", "");
	if(v.empty())
	{
		if(endsWith(getArgument_OutputPath().result, ".png"))
		{
			return ArgumentResult<Setting>(Setting_Merged);
		}
		else
		{
			return ArgumentResult<Setting>(Setting_SeperatePngs);
		}
	}
	else if(v == "seperate")
	{
		return ArgumentResult<Setting>(Setting_SeperatePngs);
	}
	else if(v == "merged")
	{
		return ArgumentResult<Setting>(Setting_Merged);
	}
	return ArgumentResult<Setting>(Setting_Merged, getValueNotValidErrorMsg("s", v));
}

bool Arguments::getArgument_ShouldShowHelp() const
{
	return hasArgument("h");
}

const Color* Arguments::getArgument_BorderColor() const
{
	if(hasArgument("b"))
	{
		string v = getArgument("b", "ff000000");
		uint32_t x;
		std::stringstream ss;
		ss << std::hex << v;
		ss >> x;
		if(v.length() == 6)
		{
			x = (x & 0xffffff) + 0xff000000;
		}
		m_cache.borderColor.fromArgb(x);
		return &m_cache.borderColor;
	}
	return nullptr;
}

bool Arguments::getArgument_Verbose() const
{
	return hasArgument("v");
}

bool Arguments::getArgument_RemoveHiddenVoxels() const
{
	return hasArgument("r");
}

vector<int> Arguments::getArgument_ClipEdges() const
{
	vector<int> distances;
	if(hasArgument("c"))
	{
		vector<string> numbers = getValueArguments(getArgument("c", string("")));
		for(auto&& numStr : numbers)
		{
			distances.push_back(numStr.empty() ? 0 : stoi(numStr));
		}
		distances.resize(6, 0);
	}
	return distances;
}

vector<int> Arguments::getArgument_ChunkSizeAndPos() const
{
	vector<int> result;
	if(hasArgument("g"))
	{
		vector<string> numbers = getValueArguments(getArgument("g", string("")));
		for(auto&& numStr : numbers)
		{
			result.push_back(numStr.empty() ? 0 : stoi(numStr));
		}
		result.resize(6, 0);
	}
	return result;
}

error Arguments::getOptionNotFoundErrorMsg(const string& optionName) const
{
	string errorMsg = "Option '";
	errorMsg += optionName;
	errorMsg += "' not found.";
	return errorMsg;
}

error Arguments::getValueNotValidErrorMsg(const string& optionName, const string& value) const
{
	string errorMsg = "Value '";
	errorMsg += value;
	errorMsg += "' is not a valid value for '";
	errorMsg += optionName;
	errorMsg += "'";
	return errorMsg;
}

bool Arguments::argumentWithoutValue(const string& key) const
{
	vector<string> keysWithoutValue = {"v", "h", "r"};
	for(auto&& v : keysWithoutValue)
	{
		if(key == v)
		{
			return true;
		}
	}
	return false;
}

void Arguments::printOptions() const
{
	printf("Options:\n");
	printf("  -i=INPUTFILE         INPUTFILE is the filepath towards the .vox file\n");
	printf("  -o=OUTPUT            OUTPUT is the directory/filepath where (all) the png('s) should be saved\n");
	printf("                          * OUTPUT can also be formatted like this: ~/output_{SIZE_X}_{SIZE_Y}.png.\n");
	printf("                            the filename will then be something like: ~/output_16_16.png.\n");
	printf("                            Formatting options are:\n");
	printf("                              SIZE_X, SIZE_Y, SIZE_Z, POS_X, POS_Y, POS_Z\n");
	printf("                              ( when -g is set: CHUNK_POS_X, CHUNK_POS_Y, CHUNK_POS_Z )\n");
	printf("  -s=SETTING           SETTING can be: 'seperate', 'merged'.\n");
	printf("                          * if its 'array' then OUTPUT should be a directory.\n");
	printf("                          * if its 'merged' then OUTPUT a png filepath.\n");
	printf("  -b=COLORHEX          COLORHEX (argb) is the color code for the borders\n");
	printf("                          * if -b is not set there will be no borders\n");
	printf("  -v                   Talk a lot.\n");
	printf("  -r                   Remove hidden voxels.\n");
	printf("  -c=DIR_DIST          DIR_DIST is an array of 6 numbers that contains the distance and direction of clipped edges.\n");
	printf("                          * Example: 'voxToPng ... -c \"(0, 0, 2, 0, 0, 0)\"'\n");
	printf("                            will clip away 2 layers from the bottom\n");
	printf("                            directions: Left, Back, Bottom, Right, Front, Top\n");
	printf("  -g=SIZE_POS          SIZE_POS is an array of 6 numbers that represent the chunk size in which to split the image up, and chunk offset pos.\n");
	printf("                          * Example: 'voxToPng ... -g \"(32, 32, -1, 0, 0, 0)\"'\n");
	printf("                            -1 = infinite size\n");
	printf("  -p=POS               SIZE is an array of 3 numbers that represent the chunk size in which to split the image up.\n");
	printf("  -h                   Show this help text.\n");
}
