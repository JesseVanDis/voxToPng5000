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


Arguments::Arguments(int argc, char **argv)
{
	string argKey;
	for(int i=0; i<argc; i++)
	{
		string arg = argv[i];
		if(arg.length() > 0 && arg[0] == '-')
		{
			argKey = arg.substr(1);
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
		return ArgumentResult<string>(m_args.at(key), ""s);
	}
	return ArgumentResult<string>(""s, getOptionNotFoundErrorMsg(key));
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
	string v = getArgument("s", ""s);
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
		string v = getArgument("b", "ff000000"s);
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

error Arguments::getOptionNotFoundErrorMsg(const string& optionName) const
{
	string errorMsg = "Option '"s;
	errorMsg += optionName;
	errorMsg += "' not found.";
	return errorMsg;
}

error Arguments::getValueNotValidErrorMsg(const string& optionName, const string& value) const
{
	string errorMsg = "Value '"s;
	errorMsg += value;
	errorMsg += "' is not a valid value for '";
	errorMsg += optionName;
	errorMsg += "'";
	return errorMsg;
}

void Arguments::printOptions() const
{
	printf("Options:");
	printf("  -i=INPUTFILE         INPUTFILE is the filepath towards the .vox file");
	printf("  -o=OUTPUT            OUTPUT is the directory/filepath where (all) the png('s) should be saved");
	printf("                          * OUTPUT can also be formatted like this: ~/output_{SIZE_X}_{SIZE_Y}.png.");
	printf("                            the filename will then be something like: ~/output_16_16.png.");
	printf("  -s=SETTING           SETTING can be: 'seperate', 'merged'.");
	printf("                          * if its 'array' then OUTPUT should be a directory.");
	printf("                          * if its 'merged' then OUTPUT a png filepath.");
	printf("  -b=COLORHEX          COLORHEX (argb) is the color code for the borders");
	printf("                          * if -b is not set there will be no borders");
	printf("  -h                   Show this help text.");
}
