#ifndef SYNCED_PRIMITIVE_BASSE_H
#define SYNCED_PRIMITIVE_BASSE_H


/**
 * @brief base class to use for synced classes
 */
class CSyncedPrimitiveBase {

	protected:

		/**
	 * @brief wrapper to call the private CSyncDebugger::Sync()
		 */
		void Sync(void* p, unsigned size, const char* op) {
#ifdef SYNCDEBUG
			CSyncDebugger::GetInstance()->Sync(p, size, op);
#endif
#ifdef SYNCCHECK
			CSyncChecker::Sync(p, size);
	#ifdef TRACE_SYNC_HEAVY
			tracefile << "Sync " << s << " " << CSyncChecker::GetChecksum() << "\n";
	#endif
#endif
		}
};

#endif
