/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

#include "../../SDK/AI.h"
#include "CGridManager.h"

namespace ntai{

	CGridManager::CGridManager() {
		DefaultCell = boost::shared_ptr<CGridCell>(new CGridCell());
		SetErrorFloat3(UpVector);
		IsInitialized=false;
		UpdateModifier=1.0f;
		UpdateModifierInterval=0;
	}

	CGridManager::~CGridManager() {
	}

	/*
	template<typename T>
	 void CGridManager::UseArray(const T* a,int size){
 		for(int i = 0; i < size; i++){
 			std::stringstream stream;
 			stream << a[i];
 			float v;
 			stream >> v;
 			SetValuebyIndex(i,v);
 		}
	}*/

	void CGridManager::ApplyModifierOnUpdate(float Modifier){
		UpdateModifier = Modifier;
	}

	void CGridManager::ApplyModifierOnUpdate(float Modifier, int Interval){
		UpdateModifier = Modifier;
		UpdateModifierInterval=Interval;
	}

	float CGridManager::GetModifierOnUpdate(){
		return UpdateModifier;
	}

	int CGridManager::GetModifierOnUpdateInterval(){
		return UpdateModifierInterval;
	}

	void CGridManager::Update() {
		GameFrame++;
		if((UpdateModifierInterval!=0)&&(UpdateModifier!=1)&&(UpdateModifier!=0)){
			if(GameFrame%UpdateModifierInterval==0){
				SlowUpdate();
			}
		}
	}

	void CGridManager::Update(int GameTime) {
		GameFrame=GameTime;
		if(GameFrame%UpdateModifierInterval==0){
			SlowUpdate();
		}
	}

	void CGridManager::SlowUpdate() {
		ApplyModifier(UpdateModifier);
	}


	bool CGridManager::Load(std::string Filename){
		return false;
	}

	bool CGridManager::Save(std::string Filename){
		return false;
	}

	void CGridManager::Rebuild() {
		boost::mutex::scoped_lock lock(cellmutex);
		if(!grid.empty()){
			grid.erase(grid.begin(),grid.end());
			grid.clear();
		}
	}

	bool CGridManager::IsValid() {
		return IsInitialized;
	}

	boost::shared_ptr<CGridCell> CGridManager::GetCell(int Index) {
		if(CellExists(Index)){
			boost::mutex::scoped_lock lock(cellmutex);
			return grid[Index];
		}else{
			return DefaultCell;
		}
	}

	int CGridManager::GetIndex(float3 Gridpos) {
		if(ValidGridPos(Gridpos)){
			int GridIndex = ((int)Gridpos.z*GetGridWidth()) + (int)Gridpos.x ;
			if(!ValidIndex(GridIndex)){
				return -1;
			}else {
				return GridIndex;
			}
		}else{
			return -1;
		}
	}

	float3 CGridManager::IndextoGrid(int Index) {
		if(ValidIndex(Index)){
			int x = 0;
			int z=0;
			x = Index%GetGridWidth();
			if(Index-x>0){
				z = (Index-x)/GetGridWidth();
			}else{
				z=0;
			}
			return float3(float(x),0,float(z));
		}else{
			return UpVector;
		}
	}

	float3 CGridManager::MaptoGrid(float3 Mappos) {
		float3 finalpos=UpVector;
		if(ValidMapPos(Mappos)){
			finalpos.z=floor(Mappos.z/GetCellHeight());
			finalpos.x=floor(Mappos.x/GetCellWidth());
			finalpos.y =0;
		}
		return finalpos;
	}

	int CGridManager::MaptoIndex(float3 Mappos) {
		if(ValidMapPos(Mappos)){
			int Index = 0;
			Index += (int)floor(Mappos.z/GetCellHeight())*GetGridWidth();
			Index += (int)floor(Mappos.x/GetCellWidth());
			return Index;
		}
		return -1;
	}

	float3 CGridManager::GridtoMap(float3 Gridpos) {
		float3 finalpos=UpVector;
		if(ValidGridPos(Gridpos)){
			finalpos.z = Gridpos.z*GetCellHeight();
			finalpos.z += GetCellHeight()/2;
			finalpos.x = Gridpos.x*GetCellWidth();
			finalpos.x += GetCellWidth()/2;
		}
		return finalpos;
	}

	void CGridManager::SetErrorFloat3(float3 ErrorPos){
		ErrorPos=CellErrorPos;
	}

	float3 CGridManager::GetErrorPos(){
		return CellErrorPos;
	}

	void CGridManager::SetDefaultGridValue(float Value){
		DefaultCell->SetValue(Value);
	}

	float CGridManager::GetDefaultGridValue(){
		return DefaultCell->GetValue();
	}

	float CGridManager::GetValue(int Index, float DefaultValue){
		boost::mutex::scoped_lock lock(cellmutex);
		map<int,boost::shared_ptr<CGridCell> >::iterator i = grid.find(Index);
		if(i == grid.end()){
			// Cell doesnt exist! It hasnt been created yet or was deleted
			// due to being of insignificant value
			if(DefaultValue == -999.0f){
				return GetDefaultGridValue();
			}else{
				return GetDefaultGridValue();
			}
		}else{
			return i->second->GetValue();
		}
	}

	float CGridManager::GetValuebyMap(float3 Mappos, float DefaultValue){
		float3 pos = MaptoGrid(Mappos);
		if(pos == GetErrorPos()){
			if(DefaultValue != -999.0f){
				return DefaultValue;
			}else{
				return GetDefaultGridValue();
			}
		}else{
			return GetValuebyGrid(pos,DefaultValue);
		}
	}

	float CGridManager::GetValuebyGrid(float3 Gridpos, float DefaultValue){
		int index = GetIndex(Gridpos);
		if(index == -1){ // impossible index returned, return the default value!!
			if(DefaultValue != -999.0f){
				return DefaultValue;
			}else{
				return GetDefaultGridValue();
			}
		}else{
			return GetValue(index, DefaultValue);
		}
	}

	void CGridManager::SetValuebyIndex(int Index, float Value) {
		if(CellExists(Index)){
			boost::mutex::scoped_lock lock(cellmutex);
			boost::shared_ptr<CGridCell> cell=grid[Index];
			if(Value < MinimumValue){
				sortedcells.erase(cell);
				grid.erase(Index);
			}else{
				cell->SetValue(Value);
				sortedcells.erase(cell);
				sortedcells.insert(cell);
			}
		}else{
			if(Value < MinimumValue){
				return;
			}
			boost::mutex::scoped_lock lock(cellmutex);
			boost::shared_ptr<CGridCell> cell(new CGridCell());
			cell->Initialize(Index);
			cell->SetValue(Value);
			grid[Index] = cell;
			sortedcells.insert(cell);
		}

	}

	void CGridManager::SetValuebyMap(float3 Mappos, float Value) {
		if(ValidMapPos(Mappos)){
			float3 GridPos = MaptoGrid(Mappos);
			if(ValidGridPos(GridPos)){
				SetValuebyGrid(GridPos, Value);
			}
		}
	}

	void CGridManager::SetValuebyGrid(float3 Gridpos, float Value) {
		if(ValidGridPos(Gridpos)){
			int Index = this->GetIndex(Gridpos);
			if(ValidIndex(Index)){
				SetValuebyIndex(Index, Value);
			}
		}
	}


		//void SetDissipator(/*some functor object that is called on update() and SetValue() to dissipate matrix values or concentrate etc*/);
		//void SetComparator(/* same again*/);



	int CGridManager::GetGridHeight() {
		return (int) floor(GridDimensions.z);
	}

	int CGridManager::GetGridWidth() {
		return (int) floor(GridDimensions.x);
	}

	int CGridManager::GetGridSize() {
		return GetGridHeight()*GetGridWidth();
	}

	float CGridManager::GetCellHeight() {
		return CellDimensions.z;
	}

	float CGridManager::GetCellWidth() {
		return CellDimensions.x;
	}

	map<int,boost::shared_ptr<CGridCell> > CGridManager::GetGrid() {
		return grid;
	}

	void CGridManager::Initialize(float3 MapDimensions, float3 GridSet, bool SizebyCell) {
		// If Size by Cell === true then create a grid where each cell is that dimension, but if it's false then create a grid of that dimension
		// and size the cells accordingly, aka a grid of 4x5 cells or a 4x5 grid of cells
		this->MapDimensions = MapDimensions;
		this->CellDimensions.y = 0;
		GridDimensions.y=0;
		if(SizebyCell){
			CellDimensions = GridSet;
			GridDimensions.x=MapDimensions.x/GridSet.x;
			GridDimensions.z=MapDimensions.z/GridSet.z;
		}else{
			GridDimensions = GridSet;
			CellDimensions.x=MapDimensions.x/GridSet.x;
			CellDimensions.z=MapDimensions.z/GridSet.z;
		}
		IsInitialized=true;
	}

	bool CGridManager::Initialized() {
		return IsInitialized;
	}

	bool CGridManager::CellExists(int Index) {
		boost::mutex::scoped_lock lock(cellmutex);
		map<int, boost::shared_ptr<CGridCell> >::iterator i = grid.find(Index);
		return (i != grid.end());
	}

	bool CGridManager::ValidGridPos(float3 Gridpos) {
		bool rvalue = true;
		if (Gridpos.z > GridDimensions.z){
			rvalue = false;
		}else if (Gridpos.x > GridDimensions.x){
			rvalue=false;
		}else if (Gridpos.z < 0){
			rvalue=false;
		}else if (Gridpos.x < 0){
			rvalue=false;
		}else if (Gridpos == ZeroVector){
			rvalue = false;
		}else if (Gridpos == UpVector){
			rvalue = false;
		}
		return rvalue;
	}
	int CGridManager::ValidGridPosE(float3 Gridpos) {
		int rvalue = 0;
		if (Gridpos.z > GridDimensions.z){
			rvalue = 1;
		}else if (Gridpos.x > GridDimensions.x){
			rvalue=2;
		}else if (Gridpos.z < 0){
			rvalue=3;
		}else if (Gridpos.x < 0){
			rvalue=4;
		}else if (Gridpos == ZeroVector){
			rvalue = 5;
		}else if (Gridpos == UpVector){
			rvalue = 6;
		}
		return rvalue;
	}
	bool CGridManager::ValidIndex(int Index) {
		bool rvalue=true;
		if (Index > GetGridSize()){
			rvalue = false;
		}else if (Index <0){
			rvalue = false;
		}
		return rvalue;
	}

	bool CGridManager::ValidMapPos(float3 MapPos){
		bool rvalue = true;
		if (MapPos.z > MapDimensions.z){
			rvalue = false;
		}else if (MapPos.x > MapDimensions.x){
			rvalue=false;
		}else if (MapPos.z < 0){
			rvalue=false;
		}else if (MapPos.x < 0){
			rvalue=false;
		}else if (MapPos == UpVector){
			rvalue=false;
		}else if (MapPos==ZeroVector){
			rvalue=false;
		}else if(MapPos.distance2D(ZeroVector)<50){
			rvalue=false;
		}
		return rvalue;
	}

	void CGridManager::ApplyModifier(float Modifier) {
		boost::mutex::scoped_lock lock(cellmutex);
		if(!grid.empty()){
			for(map<int,boost::shared_ptr<CGridCell> >::iterator i = grid.begin(); i != grid.end(); ++i){
				boost::shared_ptr<CGridCell> p = i->second;
				p->ApplyModifier(Modifier);
			}
		}
	}

	void CGridManager::ApplyModifierAtMapPos(float3 MapPos, float Modifier){
		float3 GridPos = MaptoGrid(MapPos);
		if(ValidGridPos(GridPos)){
			ApplyModifierAtGridPos(GridPos,Modifier);
		}
	}

	void CGridManager::ApplyModifierAtGridPos(float3 GridPos, float Modifier){
		int Index = GetIndex(GridPos);
		ApplyModifierAtIndex(Index,Modifier);
	}

	void CGridManager::ApplyModifierAtIndex(int Index, float Modifier){
		if(ValidIndex(Index)){
			if(CellExists(Index)){
				boost::mutex::scoped_lock lock(cellmutex);
				boost::shared_ptr<CGridCell> p =GetCell(Index);
				p->ApplyModifier(Modifier);
			}
		}
	}

	int CGridManager::GetHighestIndex() {
		if(sortedcells.empty()==false){
			boost::mutex::scoped_lock lock(cellmutex);
			boost::shared_ptr<CGridCell> p = *(sortedcells.begin());
			return p->GetIndex();
		}
		return -1;
	}

	int CGridManager::GetHighestindex(vector<float3> MapPositions) {
		if(MapPositions.empty()){
			return -1;
		}else{
			set<int> indexes;
			for(vector<float3>::iterator i = MapPositions.begin(); i != MapPositions.end(); ++i){
				//
				float3 j = *i;
				int Index = GetIndex(MaptoGrid(j));
				if(ValidIndex(Index)){
					indexes.insert(Index);
				}
			}
			{
				boost::mutex::scoped_lock lock(cellmutex);
				if(sortedcells.empty()==false){
					for(set<boost::shared_ptr<CGridCell>,cmpCell>::iterator i = sortedcells.begin(); i != sortedcells.end(); ++i){
						boost::shared_ptr<CGridCell> p = *i;
						if(indexes.find(p->GetIndex())!= indexes.end()){
							return p->GetIndex();
						}
					}
				}
			}
		}
		return -1;
	}

	int CGridManager::GetHighestindexInRadius(float3 MapPos, float Radius) {
		if(ValidMapPos(MapPos)){
			if(sortedcells.empty()==false){
				boost::mutex::scoped_lock lock(cellmutex);
				for(set<boost::shared_ptr<CGridCell>,cmpCell>::iterator i = sortedcells.begin(); i != sortedcells.end(); ++i){
					boost::shared_ptr<CGridCell> p = *i;
					float3 PMapPos = this->GridtoMap(IndextoGrid(p->GetIndex()));
					if(PMapPos.distance2D(MapPos)<Radius){
						return p->GetIndex();
					}
				}
			}
		}
		return -1;
	}
	int CGridManager::GetLowestindexInRadius(float3 MapPos, float Radius){
		if(ValidMapPos(MapPos)){
			if(sortedcells.empty()==false){
				vector<float3> values = this->GetCellsInRadius(MapPos,Radius);
				float smallest = this->MinimumValue+1;
				float3 gridpos = UpVector;
				for(vector<float3>::iterator i = values.begin(); i != values.end(); ++i){
					float v = this->GetValuebyGrid(*i);
					if(v < smallest){
						smallest = v;
						gridpos= *i;
					}
				}
				return GetIndex(gridpos);
			}
		}
		return MaptoIndex(MapPos);
	}

	void CGridManager::AddValueatMapPos(float3 MapPos, float Value){
		if(ValidMapPos(MapPos)){
			float CurrentValue = GetValuebyMap(MapPos);
			CurrentValue += Value;
			int Index = MaptoIndex(MapPos);
			if(ValidIndex(Index)){
				SetValuebyIndex(Index,CurrentValue);
			}
		}
	}

	void CGridManager::SubtractValueatMapPos(float3 MapPos, float Value){
		if(ValidMapPos(MapPos)){
			float CurrentValue = GetValuebyMap(MapPos);
			CurrentValue -= Value;
			SetValuebyMap(MapPos,CurrentValue);
		}
	}

	void CGridManager::SetMinimumValue(float Value){
		MinimumValue = Value;
	}

	float CGridManager::GetMinimumValue(){
		return MinimumValue;
	}

	vector<float3> CGridManager::GetCellsInRadius(float3 MapPos, float Radius){
		vector<float3> values;
		if(!ValidMapPos(MapPos)){
			//error_code = 98;
			return values;
		}

		//float rx = Radius/CellDimensions.x;
		//float ry = Radius/CellDimensions.z;
	//(int(Radius)%int(CellDimensions.x))//+((int)Radius%(int)CellDimensions.z)
		float3 diff = float3(Radius,0,Radius);

		float3 pbegin = MaptoGrid(MapPos - diff);

		float3 pend = MaptoGrid(MapPos + diff);

		float3 pt = pbegin;

		for(; (pt.z < pend.z)&&(pt.z < this->GridDimensions.z); pt.z++){
			for(pt.x = pbegin.x;(pt.x < pend.x)&&(pt.z < this->GridDimensions.x); pt.x++){
				if(!ValidGridPos(pt)){
					continue;
				}else{
					values.push_back(pt);
				}
			}
			
		}
		return values;
	}

	vector<float3> CGridManager::GetCellsInRadius(float3 MapPos, float Radius, int& error_code){
		vector<float3> values;
		if(!ValidMapPos(MapPos)){
			error_code = 98;
			return values;
		}

		//float rx = Radius/CellDimensions.x;
		//float ry = Radius/CellDimensions.z;
	//(int(Radius)%int(CellDimensions.x))//+((int)Radius%(int)CellDimensions.z)
		float3 diff = float3(Radius,0,Radius);

		float3 pbegin = MaptoGrid(MapPos - diff);

		float3 pend = MaptoGrid(MapPos + diff);

		float3 pt = pbegin;

		for(; (pt.z < pend.z)&&(pt.z < this->GridDimensions.z); pt.z++){
			for(pt.x = pbegin.x;(pt.x < pend.x)&&(pt.z < this->GridDimensions.x); pt.x++){
				if(!ValidGridPos(pt)){
					continue;
				}else{
					values.push_back(pt);
				}
			}
			
		}
		if(values.empty()){
			//error!
			int error_code = 3;
		}
		return values;
	}

	void CGridManager::SetCellsInRadius(float3 MapPos, float Radius, float Value){
		if(!ValidMapPos(MapPos)){
			return;
		}
		vector<float3> cellList = GetCellsInRadius(MapPos,Radius);
		if(!cellList.empty()){
			for(vector<float3>::iterator i = cellList.begin(); i != cellList.end(); ++i){
				SetValuebyGrid(*i,Value);
			}
		}
	}
}
