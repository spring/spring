#pragma once



class ModelPreloader {
public:
	static void Load();
	static void Clean();
private:
	static constexpr bool enabled = true;
private:
	static void LoadUnitDefs();
	static void LoadFeatureDefs();
	static void LoadWeaponDefs();
};