/*
 * PListHandler.cpp
 * PlistHandler configuration class implementation
 * Copyright 2006 Lorenz Pretterhofer <krysole@internode.on.net>
 */

#include "UserDefsHandler.h"

#import "Foundation/Foundation.h"

UserDefsHandler::~UserDefsHandler()
{
	[[NSUserDefaults standardUserDefaults] synchronize];
}

void UserDefsHandler::SetInt(std::string name, unsigned int value)
{
	NSNumber *number;
	NSString *key;
	
	key = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	number = [NSNumber numberWithUnsignedInt:value];
	
	[[NSUserDefaults standardUserDefaults] setObject:number forKey:key];
}

void UserDefsHandler::SetString(std::string name, std::string value)
{
	NSString *key, *val;
	
	key = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	val = [NSString stringWithCString:value.c_str() encoding:NSASCIIStringEncoding];
	
	[[NSUserDefaults standardUserDefaults] setObject:val forKey:key];
}

std::string UserDefsHandler::GetString(std::string name, std::string def)
{
	NSString *key, *value;
	
	key = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	
	value = [[NSUserDefaults standardUserDefaults] objectForKey:key];
	
	if (value == nil) {
		return def;
	}
	
	// Won't throw with plain old c-strings
	return [value cStringUsingEncoding:NSASCIIStringEncoding];
}

unsigned int UserDefsHandler::GetInt(std::string name, unsigned int def)
{
	NSString *key;
	NSNumber *number;
	
	key = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	
	number = [[NSUserDefaults standardUserDefaults] objectForKey:key];
	
	if (number == nil) {
		return def;
	}
	
	return [number unsignedIntValue];
}
