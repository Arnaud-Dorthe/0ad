/*
CStr.h
by Caecus
Caecus@0ad.wildfiregames.com

Overview:

	Contains CStr class which is a versatile class for making string use easy.
	Basic functionality implemented via STL string, so performance is limited
	based on the STL implementation we use.

Example:
	
	CStr MyString = _T("Hello, World.");
	_int MyNum = 126;
	MyString += _T(" I'm the number ") += CStr(MyNumber);
	
	// Prints "Hello, World. I'm the number 126"
	_tcout << (LPCTSTR)MyString << endl;

	MyString = _("2341");
	MyNum = MyString.ToInt();
	
	// Prints "2341"
	_tcout << MyNum << endl;	

More Info:
	http://wildfiregames.com/0ad/codepit/TDD/CStr.html

*/

#ifndef CSTR_H
#define CSTR_H

#include "Prometheus.h"
#include <string>				// Used for basic string functionality
#include <iostream>
#include <windows.h>
#include <TCHAR.h>				// TCHAR of course
#include <cstdlib>
using namespace std;

// DEFINES/ENUMS
#define CONVERSION_BUFFER_SIZE		32
#define FLOAT_CONVERSION			_T("%.6f")

#ifdef  _UNICODE
typedef wstring tstring;
#define _tcout	wcout
#define	_tstod	wcstod
#else
typedef string	tstring;
#define _tcout	cout
#define	_tstod	strtod
#endif

enum PS_TRIM_MODE {PS_TRIM_LEFT, PS_TRIM_RIGHT, PS_TRIM_BOTH};


// CStr class, the mother of all strings
class CStr
{
public:

	// CONSTRUCTORS
	CStr();					// Default constructor
	CStr(const CStr &Str);		// Copy Constructor
	
	CStr(tstring String);	// Creates CStr from C++ string
	CStr(LPCTSTR String);	// Creates CStr from C-Style TCHAR string
	CStr(TCHAR Char);		// Creates CStr from a TCHAR
	CStr(_int Number);		// Creates CStr from a int
	CStr(_uint Number);		// Creates CStr from a uint
	CStr(_long Number);		// Creates CStr from a long 
	CStr(_ulong Number);	// Creates CStr from a ulong 
	CStr(_float Number);	// Creates CStr from a float
	CStr(_double Number);	// Creates CStr from a double
	~CStr();				// Destructor

	// Conversions
	_int	ToInt();
	_uint	ToUInt();
	_long	ToLong();
	_ulong	ToULong();
	_float	ToFloat();
	_double	ToDouble();

	_long Length(){return m_String.length();}
	// Retrieves the substring within the string 
	CStr GetSubstring(_long start, _long len);

	//Search the string for another string 
	_long Find(const CStr &Str);

	//You can also do a "ReverseFind"- i.e. search starting from the end 
	_long ReverseFind(const CStr &Str);

	// Lowercase and uppercase 
	CStr LowerCase();
	CStr UpperCase();

	// Lazy funcs
	CStr LCase();
	CStr UCase();

	// Retreive the substring of the first n characters 
	CStr Left(_long len);

	// Retreive the substring of the last n characters
	CStr Right(_long len);
	
	//Remove all occurences of some character or substring 
	void Remove(const CStr &Str);

	//Replace all occurences of some substring by another 
	void Replace(const CStr &StrToReplace, const CStr &ReplaceWith);

	// Returns a trimed string, removes whitespace from the left/right/both
	CStr Trim(PS_TRIM_MODE Mode);

	// Overload operations
	CStr &operator=(const CStr &Str);
	CStr &operator=(LPCTSTR String);
	CStr &operator=(TCHAR Char);
	CStr &operator=(_int Number);
	CStr &operator=(_long Number);
	CStr &operator=(_uint Number);
	CStr &operator=(_ulong Number);
	CStr &operator=(_float Number);
	CStr &operator=(_double Number);

	_bool operator==(const CStr &Str) const;
	_bool operator!=(const CStr &Str) const;
	_bool operator<(const CStr &Str) const;
	_bool operator<=(const CStr &Str) const;
	_bool operator>(const CStr &Str) const;
	_bool operator>=(const CStr &Str) const;
	CStr &operator+=(CStr &Str);
	CStr  operator+(CStr &Str);
	operator LPCTSTR();
	TCHAR &operator[](_int n);
	TCHAR &operator[](_uint n);
	TCHAR &operator[](_long n);
	TCHAR &operator[](_ulong n);

	
private:
	tstring m_String;
	TCHAR m_ConversionBuffer[CONVERSION_BUFFER_SIZE];
};

// overloaded operator for ostreams
ostream &operator<<(ostream &os, CStr &Str);

#endif
