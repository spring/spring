#pragma once

struct DamageArray
{
	static int numTypes;
	float* damages;

	DamageArray();
	DamageArray(const DamageArray& other);
	~DamageArray();

	inline void operator=(const DamageArray& other){
		for(int a=0;a<numTypes;++a)
			damages[a]=other.damages[a];
	};
	inline float& operator[](int i){return damages[i];};
	inline float operator[](int i) const {return damages[i];};

	inline DamageArray operator*(float mul) const{
		DamageArray da(*this);
		for(int a=0;a<numTypes;++a)
			da.damages[a]*=mul;
		return da;
	}
};
