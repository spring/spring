/*
* Written by AF/T.Nowell
* www.darkstars.co.uk
* tarendai@gmail.com
*
* Written for General AI usage
* A generic Value matrixes handler
* Complete with optimizations and
* dependency handling of multiple
* matrixes whose values depend on
* each other
*/

#include <map>
#include <set>
#include <string>
#include <fstream>
#include "boost/shared_ptr.hpp"
#include "CGridCell.h"

using namespace std;

namespace ntai {

	struct cmpCell{
		bool operator() (boost::shared_ptr<CGridCell> c1, boost::shared_ptr<CGridCell> c2) const{
			return c1->GetValue() > c2->GetValue();
		}
	};


	class CGridManager{
	public:
		CGridManager();
		virtual ~CGridManager();

		/*template<typename T>
		void UseArray(const T* a,int size);*/
		template<typename T>
			void UseArray(const T* a,int size){
				for(int i = 0; i < size; i++){
					float f = (float)atof(to_string(a[i]).c_str());
					SetValuebyIndex(i,f);
				}
			}
		template<typename T>
			void UseArray(const T* a,int size, int h, int w){
				float gx,gy;
				gx = this->MapDimensions.x/w;
				gy = this->MapDimensions.z/h;
				for(int y = 0; y < h; y++){
					for(int x = 0; x < w; x++){
						float f = (float)atof(to_string(a[y*w+x]).c_str());
						this->SetValuebyMap(float3(x*gx,0,y*gy),f);
						//SetValuebyIndex(i,f);
					}
				}
			}

		template<typename T>
			void UseArrayLowValues(const T* a,int size, int h, int w){
				float gx,gy;
				gx = this->MapDimensions.x/w;
				gy = this->MapDimensions.z/h;
				for(int y = 0; y < h; y++){
					for(int x = 0; x < w; x++){
						float f = (float)atof(to_string(a[y*w+x]).c_str());
						float f2 = this->GetValuebyMap(float3(x*gx,0,y*gy));
						if(f < f2){
							this->SetValuebyMap(float3(x*gx,0,y*gy),f);
						}
						//SetValuebyIndex(i,f);
					}
				}
			}

		template<typename T>
			void UseArrayHighValues(const T* a,int size, int h, int w){
				float gx,gy;
				gx = this->MapDimensions.x/w;
				gy = this->MapDimensions.z/h;
				for(int y = 0; y < h; y++){
					for(int x = 0; x < w; x++){
						float f = (float)atof(to_string(a[y*w+x]).c_str());
						float f2 = this->GetValuebyMap(float3(x*gx,0,y*gy));
						if(f > f2){
							this->SetValuebyMap(float3(x*gx,0,y*gy),f);
						}
						//SetValuebyIndex(i,f);
					}
				}
			}

		void ApplyModifierOnUpdate(float Modifier);
		void ApplyModifierOnUpdate(float Modifier, int Interval);
		float GetModifierOnUpdate();
		int GetModifierOnUpdateInterval();
		void Update();
		void Update(int GameTime);
		void SlowUpdate();

		bool Load(std::string Filename);
		bool Save(std::string Filename);

		void Rebuild();// Destroys the grid and recreates it
		bool IsValid();



		int GetIndex(float3 Gridpos);
		float3 IndextoGrid(int Index);
		float3 GridtoMap(float3 Gridpos);
		float3 MaptoGrid(float3 Mappos);
		int MaptoIndex(float3 Mappos);

		void SetErrorFloat3(float3 ErrorPos);
		float3 GetErrorPos();

		void SetDefaultGridValue(float Value);
		float GetDefaultGridValue();

		// here the default value of DefaultValue is -999 because I dont expect to use such a number, and if that number turns out to be the one passed over then the default value specified by SetDefaultGridValue is returned. thus the second parameter is there purely as a shortcut
		float GetValue(int Index, float DefaultValue=-999.0f);// done
		float GetValuebyMap(float3 Mappos, float DefaultValue=-999.0f);// done
		float GetValuebyGrid(float3 Gridpos, float DefaultValue=-999.0f);// done

		void SetValuebyIndex(int Index, float Value);
		void SetValuebyMap(float3 Mappos, float Value);
		void SetValuebyGrid(float3 Gridpos, float Value);

		//void SetDissipator(/*some functor object that is called on update() and SetValue() to dissipate matrix values or concentrate etc*/);
		//void SetComparator(/* same again*/);


		int GetGridHeight();
		int GetGridWidth();
		int GetGridSize();
		float GetCellHeight();
		float GetCellWidth();

		map<int,boost::shared_ptr<CGridCell> > GetGrid();

		void Initialize(float3 MapDimensions, float3 GridSet, bool SizebyCell=true);
		bool Initialized();

		bool CellExists(int Index);
		bool ValidGridPos(float3 Gridpos);
		int ValidGridPosE(float3 Gridpos);
		bool ValidIndex(int Index);
		bool ValidMapPos(float3 MapPos);

		void ApplyModifier(float Modifier); // Apply to the entire grid
		void ApplyModifierAtMapPos(float3 MapPos, float Modifier); // Apply to this GridCell
		void ApplyModifierAtGridPos(float3 GridPos, float Modifier); // Apply to this GridCell
		void ApplyModifierAtIndex(int Index, float Modifier); // Apply to this GridCell

		int GetHighestIndex();
		int GetHighestindex(vector<float3> MapPositions);
		int GetHighestindexInRadius(float3 MapPos, float Radius);
		int GetLowestindexInRadius(float3 MapPos, float Radius);


		//int GetLowestindexInRadius(float3 MapPos, float Radius);

		void AddValueatMapPos(float3 MapPos, float Value);
		void SubtractValueatMapPos(float3 MapPos, float Value);

		void SetMinimumValue(float Value);
		float GetMinimumValue();

		float3 GetGridDimensions(){
			return GridDimensions;
		}

		vector<float3> GetCellsInRadius(float3 MapPos, float Radius);
		vector<float3> GetCellsInRadius(float3 MapPos, float Radius, int& error_code);

		void SetCellsInRadius(float3 MapPos, float Radius, float Value);

		boost::mutex cellmutex;
	private:
		boost::shared_ptr<CGridCell> GetCell(int Index);
		bool IsInitialized;

		std::map<int,boost::shared_ptr<CGridCell> > grid;
		std::set<boost::shared_ptr<CGridCell>,cmpCell> sortedcells;

		float3 MapDimensions;
		float3 GridDimensions;
		float3 CellDimensions;

		boost::shared_ptr<CGridCell> DefaultCell;
		float3 CellErrorPos;
		int GameFrame;
		float UpdateModifier;
		int UpdateModifierInterval;
		float MinimumValue;
	};
}
