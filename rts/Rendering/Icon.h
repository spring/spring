/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ICON_H
#define ICON_H

namespace icon {
	class CIconData;
	class CIconHandler;
	class CIcon {
		friend class CIconHandler;

		public:
			CIcon();
			CIcon(unsigned int idx);
			CIcon(const CIcon& ic);

			CIcon& operator=(const CIcon& ic);

			~CIcon();

			void UnRefData(CIconHandler* ih);

			const CIconData* operator->()  const;
			const CIconData* GetIconData() const;

		private:
			unsigned int dataIdx = 0;
	};
}

#endif // ICON_H
