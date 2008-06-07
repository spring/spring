#ifndef _PROTO_DEF
#define _PROTO_DEF


namespace netcode {
	
class ProtocolDef
{
public:
	static ProtocolDef* instance();
	
	void AddType(const unsigned char id, const int MsgLength);
	bool HasFixedLength(const unsigned char id) const;
	bool IsAllowed(const unsigned char id) const;
	
	int GetLength(unsigned char id) const;
	unsigned IsComplete(const unsigned char* const buf, const unsigned bufLength) const;

private:
	ProtocolDef();
	ProtocolDef( const ProtocolDef& );
	
	struct MsgType
	{
		int Length;
	};
	
	MsgType msg[256];
	static ProtocolDef* instance_ptr;
};

}

#endif
