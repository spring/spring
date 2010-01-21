/**
 * @file DataDirLocater.h
 * @author Tobi Vollebregt
 *
 * Copyright (C) 2006-2008 Tobi Vollebregt
 * Licensed under the terms of the GNU GPL, v2 or later
 */
#ifndef DATADIRLOCATER_H
#define DATADIRLOCATER_H

#include <string>
#include <vector>

struct DataDir
{
	DataDir(const std::string& p);
	std::string path;
	bool writable;
};

class DataDirLocater
{
public:
	DataDirLocater();
	void LocateDataDirs();

	const std::vector<DataDir>& GetDataDirs() const { return datadirs; }
	const DataDir* GetWriteDir() const { return writedir; }

private:
	std::string SubstEnvVars(const std::string& in) const;
	void AddDirs(const std::string& in);
	bool DeterminePermissions(DataDir* d);
	void DeterminePermissions();

	/**
	 * Determines whether we are in portable mode.
	 * It defines portable mode as:
	 * The spring binary (spring binary) and the synchronization
	 * library (unitsync) are in the same directory.
	 * This definition of portable mode is only valid for the data-dirs,
	 * and can not be used spring wide.
	 */
	static bool IsPortableMode();

	std::vector<DataDir> datadirs;
	const DataDir* writedir;
};

#endif // !defined(DATADIRLOCATER_H)
