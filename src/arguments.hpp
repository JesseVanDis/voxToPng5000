#ifndef VOXTOPNG_ARGUMENTS_HPP
#define VOXTOPNG_ARGUMENTS_HPP

#include <map>
#include "common.hpp"
#include "palette.hpp"

enum Setting
{
	Setting_SeperatePngs,
	Setting_Merged,
};

template <typename T>
struct ArgumentResult
{
	ArgumentResult() = default;
	explicit ArgumentResult(const T& res, error errorMsg = "")
	{
		result = res;
		err = errorMsg;
	}
	bool hasError() const { return !err.empty(); }
	bool handleError() const { if(!err.empty()) { printf("%s\n", err.c_str()); return true; } else return false; }

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
		void					addArgument(const string& key);
		void					addArgument(const string& key, const string& value);
		void					clear();

		ArgumentResult<string>	getArgument_InputPath() const;
		ArgumentResult<string>	getArgument_OutputPath() const;
		ArgumentResult<Setting>	getArgument_Setting() const;
		bool 					getArgument_ShouldShowHelp() const;
		const Color*			getArgument_BorderColor() const;
		bool 					getArgument_Verbose() const;
		bool 					getArgument_RemoveHiddenVoxels() const;
		vector<int>				getArgument_ClipEdges() const;

		bool					argumentWithoutValue(const string& key) const;

		void 					printOptions() const;

	private:
		error					getOptionNotFoundErrorMsg(const string& optionName) const;
		error					getValueNotValidErrorMsg(const string& optionName, const string& value) const;

		mutable struct
		{
			Color				borderColor;
		}					m_cache;

		map<string, string> m_args;
};


#endif //VOXTOPNG_ARGUMENTS_HPP
