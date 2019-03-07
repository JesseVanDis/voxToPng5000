//
// Created by jesse on 07.03.19.
//

#ifndef VOXTOPNG_ARGUMENTS_HPP
#define VOXTOPNG_ARGUMENTS_HPP

#include <map>
#include "common.hpp"

enum Setting
{
	Setting_SeperatePngs,
	Setting_Merged,
};

template <typename T>
struct ArgumentResult
{
	ArgumentResult() = default;
	explicit ArgumentResult(const T& res, error errorMsg = ""s)
	{
		result = res;
		err = errorMsg;
	}
	bool hasError() const { return err != ""s; }
	bool handleError() const { if(err != ""s) { printf("%s\n", err.c_str()); return true; } else return false; }

	error err;
	T result;
};

class Arguments
{
	public:
		Arguments(int argc, char **argv);
		string 					getArgument(const string& key, const string& defaultValue) const;
		ArgumentResult<string> 	getArgument(const string& key) const;
		bool 					hasArgument(const string& key) const;

		ArgumentResult<string>	getArgument_InputPath() const;
		ArgumentResult<string>	getArgument_OutputPath() const;
		ArgumentResult<Setting>	getArgument_Setting() const;
		bool 					getArgument_ShouldShowHelp() const;

		void 					printOptions() const;

	private:
		error					getOptionNotFoundErrorMsg(const string& optionName) const;
		error					getValueNotValidErrorMsg(const string& optionName, const string& value) const;

		map<string, string> m_args;
};


#endif //VOXTOPNG_ARGUMENTS_HPP
