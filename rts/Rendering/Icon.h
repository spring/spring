/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ICON_H
#define ICON_H

namespace icon {
	class CIconData;
	class CIcon {
		friend class CIconHandler;

		public:
			CIcon();
			CIcon(CIconData* data);
			CIcon(const CIcon& ic);
			CIcon& operator=(const CIcon& ic);
			~CIcon();

			const CIconData* operator->()  const { return data; }
			const CIconData* GetIconData() const { return data; }

		private:
			CIconData* data;
	};
}

#endif // ICON_H
