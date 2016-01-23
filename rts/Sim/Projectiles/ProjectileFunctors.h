/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#ifndef PROJECTILE_FUNCTORS_HDR
#define PROJECTILE_FUNCTORS_HDR

class CProjectile;
struct FlyingPiece;


struct ProjectileDistanceComparator {
	bool operator() (const CProjectile* arg1, const CProjectile* arg2) const;
};

struct FlyingPieceComparator {
	bool operator() (const FlyingPiece& fp1, const FlyingPiece& fp2) const;
};

#endif

