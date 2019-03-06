
#ifndef VOXTOPNG_DICTIONARY_HPP
#define VOXTOPNG_DICTIONARY_HPP

#include <map>
#include <vector>
#include "common.hpp"

class Loader;

using namespace std;

class Dictionary
{
	public:
		Dictionary();
		error 			load(Loader& loader);

		bool			hasKey(const string& key) const;
		string 			getValue(const string& key, const string& defaultValue) const;
		int 			getValue(const string& key, int defaultValue) const;
		vector<int> 	getValues(const string& key, const vector<int>& defaultValues) const;

	private:
		map<string, string> m_data;
};


#endif //VOXTOPNG_DICTIONARY_HPP
