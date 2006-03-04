
#ifndef SERIALIZE_IFACE_H
#define SERIALIZE_IFACE_H

namespace creg
{
	class Class;

	class ISerializer
	{
	public:
		virtual bool IsWriting () = 0;
		virtual void Serialize (void *data, int byteSize) = 0;

		// Serialize a pointer to an object.
		virtual void SerializeObjectPtr (void **ptr, Class *objectClass) = 0;
		
		// Serialize an instance of an object, could be a structure embedded into another object
		virtual void SerializeObjectInstance (void *inst, Class *objectClass) = 0;

		// useful helper operators
		ISerializer& operator&(int& v) { Serialize(&v, sizeof(int)); return *this; }
		ISerializer& operator&(long& v) { Serialize(&v, sizeof(long)); return *this; }
		ISerializer& operator&(short& v) { Serialize(&v, sizeof(short)); return *this; }
		ISerializer& operator&(char& v) { Serialize(&v, sizeof(char)); return *this; }

		ISerializer& operator&(unsigned int& v) { Serialize(&v, sizeof(int)); return *this; }
		ISerializer& operator&(unsigned long& v) { Serialize(&v, sizeof(long)); return *this; }
		ISerializer& operator&(unsigned short& v) { Serialize(&v, sizeof(short)); return *this; }
		ISerializer& operator&(unsigned char& v) { Serialize(&v, sizeof(char)); return *this; }

	protected:
		// Helper for derived classes, directly calls serialize on the members
		void SerializeClassInstance (void *ptr, creg::Class *cls);
	};

};

#endif
