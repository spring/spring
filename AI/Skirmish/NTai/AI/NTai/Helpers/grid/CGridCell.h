#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace ntai {
	class CGridCell{
	public:
		CGridCell();
		virtual ~CGridCell();

		void Initialize(int Index);
		bool IsValid();

		std::string toString();
		void FromString(std::string s);

		float GetValue();
		int GetIndex();
		void SetValue(float Value);
		void SetIndex(int i);

		int GetLastChangeTime();
		bool SetLastChangeTime(int TimeFrame);

		void ApplyModifier(float Modifier);
	private:
		boost::mutex cell_mutex;
		float CellValue;
		int Index;
		int ChangeTime;
	};
}
