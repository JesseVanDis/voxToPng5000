
#include <cstring>
#include "dictionary.hpp"
#include "loader.hpp"

static int readNumberFromString(const char* pString, int skip, int defaultVal, bool* pSuccess = nullptr) {
	char numberStr[128];
	memset(numberStr, 0, sizeof(numberStr));
	int n = 0;
	size_t len = strlen(pString);
	for(size_t i=0; i<len; i++)
	{
		const char c = pString[i];
		int isNumber = (c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9' || c == '-') ? 1 : 0;
		if(skip == 0)
		{
			if(isNumber == 1)
			{
				numberStr[n++] = c;
			}
			else
			{
				break;
			}
		}
		else if(isNumber == 0)
		{
			skip--;
		}
	}
	if(numberStr[0] == '\0')
	{
		if(pSuccess != nullptr)
		{
			*pSuccess = false;
		}
		return defaultVal;
	}
	if(pSuccess != nullptr)
	{
		*pSuccess = true;
	}
	return atoi(numberStr);
}


Dictionary::Dictionary()
{

}

error Dictionary::load(Loader& loader)
{
	int32_t numKeyValuePairs = loader.readNextInt32();
	for(size_t i=0; i<numKeyValuePairs; i++)
	{
		int32_t keyStringSize = loader.readNextInt32();
		vector<char> stringVec = loader.readNextChars(keyStringSize);
		stringVec.push_back('\0');
		string key = stringVec.data();

		int32_t valueSize = loader.readNextInt32();
		vector<char> valueVec = loader.readNextChars(valueSize);
		valueVec.push_back('\0');
		string value = valueVec.data();
		m_data[key] = value;
	}

	return ""s;
}

bool Dictionary::hasKey(const string& key) const
{
	return m_data.count(key) > 0;
}

string Dictionary::getValue(const string& key, const string& defaultValue) const
{
	if(hasKey(key))
	{
		return m_data.at(key);
	}
	return defaultValue;
}

int Dictionary::getValue(const string& key, int defaultValue) const
{
	if(hasKey(key))
	{
		return readNumberFromString(m_data.at(key).c_str(), 0, defaultValue);
	}
	return defaultValue;
}

vector<int> Dictionary::getValues(const string& key, const vector<int>& defaultValues) const
{
	if(!hasKey(key))
	{
		return defaultValues;
	}
	const string& value = m_data.at(key);
	vector<int> numbers;
	numbers.reserve(defaultValues.size() + 16);
	for(size_t i=0; i<10000; i++)
	{
		int defaultValue = i < defaultValues.size() ? defaultValues[i] : 0;
		bool success = false;
		int number = readNumberFromString(value.c_str(), i, defaultValue, &success);
		if(!success)
		{
			break;
		}
		numbers.push_back(number);
	}
	return numbers;
}


/*

int readDict(const char* buffer, dictionary* pOutput)
{
    int numKeyValuePairs = *((int*)&buffer[0]);
	pOutput->num = numKeyValuePairs;
    int next = 4;
    for(size_t i=0; i<numKeyValuePairs; i++)
    {
        int keyStringSize = *((int*)&buffer[next]);
        const char* keyString = &buffer[next+4];
        next += keyStringSize + 4;
        int valueStringSize = *((int*)&buffer[next]);
        const char* valueString = &buffer[next+4];
        next += valueStringSize + 4;

        int warnKey = 0;
        int warnValue = 0;
        if(keyStringSize > DICTIONARY_KEY_STRING_CAPACITY)
        {
            keyStringSize = DICTIONARY_KEY_STRING_CAPACITY-1;
            warnKey = 1;
        }
        if(keyStringSize > DICTIONARY_KEY_STRING_CAPACITY)
        {
            warnValue = 1;
            valueStringSize = DICTIONARY_KEY_STRING_CAPACITY-1;
        }

        if(i<DICTIONARY_KEY_CAPACITY)
        {
            memset(pOutput->keys[i], 0, DICTIONARY_KEY_STRING_CAPACITY);
            memset(pOutput->values[i], 0, DICTIONARY_KEY_STRING_CAPACITY);
            memcpy(pOutput->keys[i], keyString, keyStringSize);
            memcpy(pOutput->values[i], valueString, valueStringSize);

            if(warnKey == 1)
            {
                printf("warn: the length of key %s cannot be bigger than %d characters\n", pOutput->keys[i], (int)DICTIONARY_KEY_STRING_CAPACITY);
            }
            if(warnValue == 1)
            {
                printf("warn: the length of value %s cannot be bigger than %d characters\n", pOutput->values[i], (int)DICTIONARY_KEY_STRING_CAPACITY);
            }
        }
        else
        {
            printf("warn: failed to extract all keys of dict\n");
        }
    }
    return next;
}

 */