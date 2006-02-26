/*
 * PListHandler.cpp
 * PlistHandler configuration class implementation
 * Copyright 2006 Lorenz Pretterhofer <krysole@internode.on.net>
 */

#include "UserDefsHandler.h"

#import "Foundation/Foundation.h"

UserDefsHandler::UserDefsHandler()
{
	NSUserDefaults* defaults;
	NSString *bundleIdentifier;
	NSDictionary *defDict;

	bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
	defaults = [NSUserDefaults standardUserDefaults];
	
	defDict = [defaults persistentDomainForName:bundleIdentifier];
	
	dictionary_ = [[NSDictionary dictionaryWithDictionary:defDict] retain];
}

UserDefsHandler::~UserDefsHandler()
{
	NSUserDefaults *defaults;
	NSString *bundleIdentifier;
	
	bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
	defaults = [NSUserDefaults standardUserDefaults];
	
	[defaults setPersistentDomain:dictionary_ forName:bundleIdentifier];
	
	[dictionary_ release];
}

void UserDefsHandler::SetInt(std::string name, unsigned int value)
{
	NSNumber *number;
	NSString *key;
	
	key = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	number = [NSNumber numberWithUnsignedInt:value];
	
	[dictionary_ setObject:number forKey:key];
}

void UserDefsHandler::SetString(std::string name, std::string value)
{
	NSString *key, *val;
	
	key = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	val = [NSString stringWithCString:value.c_str() encoding:NSASCIIStringEncoding];
	
	[dictionary_ setObject:val forKey:key];
}

std::string UserDefsHandler::GetString(std::string name, std::string def)
{
	NSString *key, *value;
	
	key = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	value = [dictionary_ objectForKey:key];
	
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
	number = [dictionary_ objectForKey:key];
	
	if (number == nil) {
		return def;
	}
	
	return [number unsignedIntValue];
}
