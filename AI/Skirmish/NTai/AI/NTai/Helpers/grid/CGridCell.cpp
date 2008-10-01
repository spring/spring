#include "CGridCell.h"

namespace ntai {

	CGridCell::CGridCell(){
		
	}

	CGridCell::~CGridCell(){
		
	}

	void CGridCell::Initialize(int Index){
		this->Index=Index;
	}

	bool CGridCell::IsValid(){
		return (Index != -1);
	}
	std::string CGridCell::toString() {
		return "";
	}

	void CGridCell::FromString(std::string s) {
		//
	}

	float CGridCell::GetValue(){
		boost::mutex::scoped_lock lock(cell_mutex);
		return CellValue;
	}

	int CGridCell::GetIndex(){
		return Index;
	}

	void CGridCell::SetValue(float Value){
		boost::mutex::scoped_lock lock(cell_mutex);
		CellValue=Value;
	}

	void CGridCell::SetIndex(int i){
		Index = i;
	}

	int CGridCell::GetLastChangeTime(){
		boost::mutex::scoped_lock lock(cell_mutex);
		return ChangeTime;
	}

	bool CGridCell::SetLastChangeTime(int TimeFrame){
		boost::mutex::scoped_lock lock(cell_mutex);
		ChangeTime=TimeFrame;
		return true;
	}

	void CGridCell::ApplyModifier(float Modifier){
		boost::mutex::scoped_lock lock(cell_mutex);
		CellValue *= Modifier;
	}

}
