typedef void( PLATFORMCALLINGCONVENTION *INITAI)( void * aicallback,int team);
typedef void( PLATFORMCALLINGCONVENTION *UPDATE)( );
typedef void( PLATFORMCALLINGCONVENTION *GOTCHATMSG)( const char * msg,int priority);
typedef void( PLATFORMCALLINGCONVENTION *UNITCREATED)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *UNITFINISHED)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *UNITIDLE)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *UNITMOVEFAILED)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *UNITDAMAGED)( int damaged,int attacker,float damage,float dirx,float diry,float dirz);
typedef void( PLATFORMCALLINGCONVENTION *UNITDESTROYED)( int enemy,int attacker);
typedef void( PLATFORMCALLINGCONVENTION *ENEMYENTERLOS)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *ENEMYLEAVELOS)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *ENEMYENTERRADAR)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *ENEMYLEAVERADAR)( int unit);
typedef void( PLATFORMCALLINGCONVENTION *ENEMYDAMAGED)( int damaged,int attacker,float damage,float dirx,float diry,float dirz);
typedef void( PLATFORMCALLINGCONVENTION *ENEMYDESTROYED)( int enemy,int attacker);

