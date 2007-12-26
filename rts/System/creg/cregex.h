
#define CREX_REG_STATE_COLLECTOR(Name,RootClass) \
static RootClass *Name##State;\
\
class Name##StateCollector {\
	CR_DECLARE(Name##StateCollector);\
	void Serialize(creg::ISerializer *s);\
};\
\
CR_BIND(Name##StateCollector, );\
\
CR_REG_METADATA(Name##StateCollector,(CR_SERIALIZER(Serialize)));\
\
void Name##StateCollector::Serialize(creg::ISerializer *s)\
{\
	s->SerializeObjectInstance(Name##State,Name##State->GetClass());\
}

#define CREX_SC_LOAD(Name,ifs)\
	creg::CInputStreamSerializer iss;\
	Name##State = this;\
	Name##StateCollector *sc;\
	void *psc = 0;\
	creg::Class *sccls;\
	iss.LoadPackage(ifs, psc, sccls);\
	assert (psc && sccls == Name##StateCollector::StaticClass());\
	sc = (Name##StateCollector*)psc;\
	Name##State = 0;

#define CREX_SC_SAVE(Name,ofs)\
	creg::COutputStreamSerializer oss;\
	Name##State = this;\
	Name##StateCollector sc;\
	oss.SavePackage(ofs, &sc, sc.GetClass());\
	Name##State = 0;
