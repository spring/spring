void SetInitAICallback(void *fnpointer )
{
   WriteLine( "SetInitAICallback >>>" );
   thisaiproxy->InitAI = ( INITAI )fnpointer;
   WriteLine( "SetInitAICallback <<<" );
}
void SetUpdateCallback(void *fnpointer )
{
   WriteLine( "SetUpdateCallback >>>" );
   thisaiproxy->Update = ( UPDATE )fnpointer;
   WriteLine( "SetUpdateCallback <<<" );
}
void SetGotChatMsgCallback(void *fnpointer )
{
   WriteLine( "SetGotChatMsgCallback >>>" );
   thisaiproxy->GotChatMsg = ( GOTCHATMSG )fnpointer;
   WriteLine( "SetGotChatMsgCallback <<<" );
}
void SetUnitCreatedCallback(void *fnpointer )
{
   WriteLine( "SetUnitCreatedCallback >>>" );
   thisaiproxy->UnitCreated = ( UNITCREATED )fnpointer;
   WriteLine( "SetUnitCreatedCallback <<<" );
}
void SetUnitFinishedCallback(void *fnpointer )
{
   WriteLine( "SetUnitFinishedCallback >>>" );
   thisaiproxy->UnitFinished = ( UNITFINISHED )fnpointer;
   WriteLine( "SetUnitFinishedCallback <<<" );
}
void SetUnitIdleCallback(void *fnpointer )
{
   WriteLine( "SetUnitIdleCallback >>>" );
   thisaiproxy->UnitIdle = ( UNITIDLE )fnpointer;
   WriteLine( "SetUnitIdleCallback <<<" );
}
void SetUnitMoveFailedCallback(void *fnpointer )
{
   WriteLine( "SetUnitMoveFailedCallback >>>" );
   thisaiproxy->UnitMoveFailed = ( UNITMOVEFAILED )fnpointer;
   WriteLine( "SetUnitMoveFailedCallback <<<" );
}
void SetUnitDamagedCallback(void *fnpointer )
{
   WriteLine( "SetUnitDamagedCallback >>>" );
   thisaiproxy->UnitDamaged = ( UNITDAMAGED )fnpointer;
   WriteLine( "SetUnitDamagedCallback <<<" );
}
void SetUnitDestroyedCallback(void *fnpointer )
{
   WriteLine( "SetUnitDestroyedCallback >>>" );
   thisaiproxy->UnitDestroyed = ( UNITDESTROYED )fnpointer;
   WriteLine( "SetUnitDestroyedCallback <<<" );
}
void SetEnemyEnterLOSCallback(void *fnpointer )
{
   WriteLine( "SetEnemyEnterLOSCallback >>>" );
   thisaiproxy->EnemyEnterLOS = ( ENEMYENTERLOS )fnpointer;
   WriteLine( "SetEnemyEnterLOSCallback <<<" );
}
void SetEnemyLeaveLOSCallback(void *fnpointer )
{
   WriteLine( "SetEnemyLeaveLOSCallback >>>" );
   thisaiproxy->EnemyLeaveLOS = ( ENEMYLEAVELOS )fnpointer;
   WriteLine( "SetEnemyLeaveLOSCallback <<<" );
}
void SetEnemyEnterRadarCallback(void *fnpointer )
{
   WriteLine( "SetEnemyEnterRadarCallback >>>" );
   thisaiproxy->EnemyEnterRadar = ( ENEMYENTERRADAR )fnpointer;
   WriteLine( "SetEnemyEnterRadarCallback <<<" );
}
void SetEnemyLeaveRadarCallback(void *fnpointer )
{
   WriteLine( "SetEnemyLeaveRadarCallback >>>" );
   thisaiproxy->EnemyLeaveRadar = ( ENEMYLEAVERADAR )fnpointer;
   WriteLine( "SetEnemyLeaveRadarCallback <<<" );
}
void SetEnemyDamagedCallback(void *fnpointer )
{
   WriteLine( "SetEnemyDamagedCallback >>>" );
   thisaiproxy->EnemyDamaged = ( ENEMYDAMAGED )fnpointer;
   WriteLine( "SetEnemyDamagedCallback <<<" );
}
void SetEnemyDestroyedCallback(void *fnpointer )
{
   WriteLine( "SetEnemyDestroyedCallback >>>" );
   thisaiproxy->EnemyDestroyed = ( ENEMYDESTROYED )fnpointer;
   WriteLine( "SetEnemyDestroyedCallback <<<" );
}

