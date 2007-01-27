#ifndef __DAMAGE_ARRAY_H__
#define __DAMAGE_ARRAY_H__

struct DamageArray
{
	CR_DECLARE_STRUCT(DamageArray);
	static int numTypes;
	float* damages;
	int paralyzeDamageTime;
	float impulseFactor, impulseBoost, craterMult, craterBoost;

	DamageArray();
	DamageArray(const DamageArray& other);
	~DamageArray();

	inline void operator=(const DamageArray& other){
		paralyzeDamageTime=other.paralyzeDamageTime;
		impulseFactor=other.impulseFactor;
		impulseBoost=other.impulseBoost;
		craterMult=other.craterMult;
		craterBoost=other.craterBoost;
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

private:
	void creg_Serialize(creg::ISerializer& s);
};

#endif // __DAMAGE_ARRAY_H__
