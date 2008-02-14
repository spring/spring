#include "EventPrinter.h"

#include <iostream>

using namespace std;

EventPrinter::~EventPrinter()
{
}

void EventPrinter::Message(const std::string& message)
{
	cout << message << endl;
}

void EventPrinter::Warning(const std::string& message)
{
	cout << message << endl;
}
