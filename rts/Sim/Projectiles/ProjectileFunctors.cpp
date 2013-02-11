/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "ProjectileFunctors.h"
#include "Projectile.h"
#include "Unsynced/FlyingPiece.h"

void ProjectileDetacher::Detach(CProjectile* p) {
	p->Detach();
}

int ProjectileIndexer::Index(const CProjectile* p) {
	return p->id;
}



bool ProjectileDistanceComparator::operator() (const CProjectile* arg1, const CProjectile* arg2) const {
	if (arg1->tempdist != arg2->tempdist) // strict ordering required
		return (arg1->tempdist > arg2->tempdist);
	return (arg1 > arg2);
}

bool FlyingPieceComparator::operator() (const FlyingPiece* fp1, const FlyingPiece* fp2) const {
	if (fp1->GetTexture() != fp2->GetTexture())
		return (fp1->GetTexture() > fp2->GetTexture());
	if (fp1->GetTeam() != fp2->GetTeam())
		return (fp1->GetTeam() > fp2->GetTeam());
	return (fp1 > fp2);
}

