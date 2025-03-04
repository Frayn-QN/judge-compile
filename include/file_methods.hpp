#pragma once

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace boost::archive::iterators;
namespace fs = boost::filesystem;

class Base64 {
public:
    Base64() {}
    ~Base64() {}

    /**
     * @brief 将文件编码为 Base64 字符串
     * @param filePath 文件路径
     * @param output 输出的 Base64 字符串
     */
	static void EncodeFileToBase64(const fs::path &filePath, string &output) {
        ifstream file(filePath, ios::binary);
        if (!file) {
            throw runtime_error("Cannot open file for reading");
        }

        stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        string input = buffer.str();
        if (!Base64Encode(input, &output)) {
            throw runtime_error("Base64 encoding failed");
        }
    }

    /**
     * @brief 将 Base64 字符串解码并写入文件
     * @param base64str Base64 字符串
     * @param outputFilePath 输出文件路径
     */
    static void DecodeBase64ToFile(const string &base64str, const fs::path &outputFilePath) {
        string output;
        if (!Base64Decode(base64str, &output)) {
            throw runtime_error("Base64 decoding failed");
        }

        ofstream file(outputFilePath, ios::binary);
        if (!file) {
            throw runtime_error("Cannot open file for writing");
        }

        file.write(output.c_str(), output.size());
        file.close();
    }
    

private:
    static bool Base64Encode( const string &input, string *output)
    {
    	typedef base64_from_binary<transform_width<string::const_iterator, 6, 8>> Base64EncodeIterator;
    	stringstream result;
    	try {
    		copy( Base64EncodeIterator( input.begin() ), Base64EncodeIterator( input.end() ), ostream_iterator<char>( result ) );
    	} catch ( ... ) {
    		return false;
    	}
    	size_t equal_count = (3 - input.length() % 3) % 3;
    	for ( size_t i = 0; i < equal_count; i++ )
    	{
    		result.put( '=' );
    	}
    	*output = result.str();
    	return output->empty() == false;
    }
 
    static bool Base64Decode( const string &input, string *output)
    {
    	typedef transform_width<binary_from_base64<string::const_iterator>, 8, 6> Base64DecodeIterator;
    	stringstream result;
    	try {
    		copy( Base64DecodeIterator( input.begin() ), Base64DecodeIterator( input.end() ), ostream_iterator<char>( result ) );
    	} catch ( ... ) {
    		return false;
    	}
    	*output = result.str();
    	return output->empty() == false;
    }
};