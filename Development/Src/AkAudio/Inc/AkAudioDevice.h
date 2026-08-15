#ifndef _INC_AKAUDIODEVICE
#define _INC_AKAUDIODEVICE

#include "AkWwise.EXCERPT.h"
#include "Map.h"

class UAkComponent;
class IInterface_AkEventHandler;

#define DUMMY_GAMEOBJ ((AkGameObjectID)0x2)


namespace AK {
	namespace SoundFrame {
		class IClient;
		class ISoundFrame;
		class CSoundFrameUED;
		bool CreateUEd( IClient * in_pClient, ISoundFrame ** out_ppSoundFrame );
	}
}
/*------------------------------------------------------------------------------------
	UAkAudioDevice
------------------------------------------------------------------------------------*/

class UAkAudioDevice 
	: public USubsystem
#ifdef AK_SOUNDFRAME
	, public AK::SoundFrame::IClient
#endif
{
	DECLARE_CLASS_INTRINSIC(UAkAudioDevice,USubsystem,CLASS_Config|CLASS_Transient|CLASS_Intrinsic,AkAudio)

	// Constructor.
	void StaticConstructor();

	// UAudioDevice interface.
	virtual UBOOL Init();
	virtual void Update( UBOOL bGameTicking );
	virtual void FinishDestroy();
	virtual void ShutdownAfterError();

	/**
	 * Sets the listener's location and orientation for the viewport
	 */
	void SetListener( INT ViewportIndex, const FVector& Location, const FVector& Up, const FVector& Right, const FVector& Front );
	void SetObjectPosition( const FVector& Location, const FVector& Rotation, UAkComponent* pComponent );
	void SetObjectMultiPositions( const TArray<FVector>& Locations, const TArray<FVector>& Rotations, UAkComponent* pComponent ); // rcharpentier
	void StopAllSounds( UBOOL bShouldStopUISounds = FALSE );

	// Replication of Sound Engine interface for easy rewiring to SoundFrame

	AKRESULT ClearBanks();

	AKRESULT LoadBank(
		const TCHAR *       in_pszString,			///< Name of the bank to load
		AkMemPoolId         in_memPoolId,			///< Memory pool ID (media is stored in the sound engine's default pool if AK_DEFAULT_POOL_ID is passed)
		AkBankID &          out_bankID				///< Returned bank ID
		);

	AKRESULT LoadBank(
        const TCHAR *       in_pszString,           ///< Name/path of the bank to load
		AkBankCallbackFunc  in_pfnBankCallback,	    ///< Callback function
		void *              in_pCookie,				///< Callback cookie (reserved to user, passed to the callback function)
        AkMemPoolId         in_memPoolId,			///< Memory pool ID (media is stored in the sound engine's default pool if AK_DEFAULT_POOL_ID is passed)
		AkBankID &          out_bankID				///< Returned bank ID
        );

	AKRESULT UnloadBank(
        const TCHAR *       in_pszString,			///< Name of the bank to unload
        AkMemPoolId *       out_pMemPoolId = NULL   ///< Returned memory pool ID used with LoadBank() (can pass NULL)
        );

	AKRESULT UnloadBank(
        const TCHAR *       in_pszString,           ///< Name of the bank to unload
		AkBankCallbackFunc  in_pfnBankCallback,	    ///< Callback function
		void *              in_pCookie 				///< Callback cookie (reserved to user, passed to the callback function)
        );

	AKRESULT LoadInitBank();
	AKRESULT UnloadInitBank();

	void LoadAllReferencedBanks();
	void ReloadAllReferencedBanks();

	UBOOL ConditionalReloadAllBanks(UBOOL bForDLC); // rcharpentier - returns whether banks were reloaded
	void SetConsoleDLCPath(const FString& dlcPath) { ConsoleDLCPath = dlcPath; }

	void RegisterAkEventHandler(FName in_PackageName, class IInterface_AkEventHandler* in_pHandler);
	void UnregisterAkEventHandler(FName in_PackageName, class IInterface_AkEventHandler* in_pHandler);

	enum { ForcePlayCookie = 0xB00B1E5 }; // hack to force playing a matinee sounds, passed in as the cookie for a NULL callback

	AkPlayingID PostEvent(
		class UAkEvent * in_pEvent, 
		AActor * in_pActor, 
		FName in_nameBone, 
        AkUInt32 in_uFlags = 0,						///< Bitmask: see \ref AkCallbackType
		AkCallbackFunc in_pfnCallback = NULL,		///< Callback function
		void * in_pCookie = NULL,					///< Callback cookie that will be sent to the callback function along with additional information.
		bool in_bStopWhenOwnerDestroyed = false,
		bool in_checkHandler = true
        );

// Begin Outlast Change - plalonde
	void StopPlayingID(
		AkPlayingID in_playingID,
		AkTimeMs in_uTransitionDuration = 0,
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear
		);
// End Outlast Change

// Begin Outlast Change - plalonde
	enum AkActionType
	{
		AkAction_Stop = 0,
		AkAction_Pause = 1,
		AkAction_Resume = 2,
		AkAction_Break = 3
	};

	AKRESULT ExecuteActionOnEvent(
		class UAkEvent * in_pEvent, 
		AkActionType in_action,
		AActor * in_pActor, 
		FName in_nameBone = NAME_None, 
		AkTimeMs in_uTransitionDuration = 0, 
		AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear, 
		AkPlayingID in_playingID = AK_INVALID_PLAYING_ID
		);
// End Outlast Change

	AKRESULT PostTrigger( 
		const TCHAR * in_pszTrigger,				///< Name of the trigger
		AActor * in_pActor,							///< Associated game object ID
		FName in_nameBone = NAME_None
		);

	AKRESULT SetRTPCValue( 
		const TCHAR * in_pszRtpcName,				///< Name of the RTPC
		AkRtpcValue in_value, 						///< Value to set
		AActor * in_pActor,							///< Associated game object ID
		FName in_nameBone = NAME_None
		);

	AKRESULT SetState( 
		const TCHAR * in_pszStateGroup,				///< Name of the state group
		const TCHAR * in_pszState 					///< Name of the state
	    );

	AKRESULT SetSwitch( 
		const TCHAR * in_pszSwitchGroup,			///< Name of the switch group
		const TCHAR * in_pszSwitchState, 			///< Name of the switch
		AActor * in_pActor,							///< Associated game object ID
		FName in_nameBone = NAME_None
		);

	AKRESULT ActivateOcclusion(
		bool in_bActivate,
		AActor * in_pActor, 
		FName in_nameBone = NAME_None
		);

	AKRESULT SetObstructionAndOcclusion(
		FLOAT obstruction,
		FLOAT occlusion,
		AActor * in_pActor,
		FName in_nameBone = NAME_None
		);

	AKRESULT SetObstructionAndOcclusionOnAllComponents(
		FLOAT obstruction,
		FLOAT occlusion,
		AActor * in_pActor
		);

	// plalonde - For Reverb
	struct AkAuxBusValue
	{
		const TCHAR *	AuxBus;
		AkReal32		ControlValue;
	};

	AKRESULT SetGameObjectAuxSendValues(
		const TArray<AkAuxBusValue>& in_auxValues,
		AActor * in_pActor,
		FName in_nameBone = NAME_None
		);

	AKRESULT SetGameObjectAuxSendValuesOnAllComponents(
		const TArray<AkAuxBusValue>& in_auxValues,
		AActor * in_pActor
		);

	AKRESULT SetGameObjectOutputBusVolume(
		AkReal32 in_volume,
		AActor * in_pActor,
		FName in_nameBone = NAME_None
		);

	void GetProjectAudioFileRoot(
		FString& out_sRoot
		);

#ifdef AK_SOUNDFRAME
	// IClient
	/// Notification for connection status changes. This method is called after connection to or disconnection from the Wwise application occurs.
	virtual void OnConnect( 
		bool in_bConnect		///< True if Wwise is connected, False if it is not
		);

	/// Event notification. This method is called when an event is added, removed, changed, or pushed.
	virtual void OnEventNotif( 
		Notif in_eNotif,			///< Notification type
		AkUniqueID in_eventID		///< Unique ID of the event
		) {}

	/// Dialogue Event notification. This method is called when a dialogue event is added, removed or changed.
	/// \aknote
	/// This notification will be sent if an argument is added, removed or moved within a dialogue event.
	/// \endaknote
	virtual void OnDialogueEventNotif( 
		Notif in_eNotif,					///< Notification type
		AkUniqueID in_dialogueEventID		///< Unique ID of the dialogue event
		) {}

	/// Sound object notification. This method is called when a sound object is added, removed, or changed.
	virtual void OnSoundObjectNotif( 
		Notif in_eNotif,					///< Notification type
		AkUniqueID in_soundObjectID			///< Unique ID of the sound object
		);

	/// State notification. This method is called when a state group or a state is added, removed or changed.\n
	/// It is also called (with in_eNotif equal to Notif_Changed) when the current state of a state group changes.
	/// \aknote
	/// This notification will be sent for all state changes (through Wwise, the Sound Frame, or the sound engine).
	/// \endaknote
	virtual void OnStatesNotif( 
		Notif in_eNotif,			///< Notification type
		AkUniqueID in_stateGroupID	///< Unique ID of the state group
		) {}

	/// Switch notification. This method is called when a switch group or a switch is added, removed or changed.\n
	/// It is also called (with in_eNotif equal to Notif_Changed) when the current switch in a switch group changes on any game object.\n
	/// \aknote
	/// This notification will be sent for all switch changes (through Wwise, the Sound Frame, or the sound engine).
	/// \endaknote
	/// When all switches are reset from Wwise, this method will be called once with in_eNotif equal to Notif_Reset and 
	/// in_switchGroupID equal to AK_INVALID_UNIQUE_ID. This means that all switches have been reset to their default value
	/// on all game objects.
	virtual void OnSwitchesNotif( 
		Notif in_eNotif,			///< Notification type
		AkUniqueID in_switchGroupID	///< Unique ID of the switch group
		) {}

	/// Game parameter notification. This method is called when a game parameter is added, removed, or changed.
	virtual void OnGameParametersNotif( 
		Notif in_eNotif,				///< Notification type
		AkUniqueID in_gameParameterID	///< Unique ID of the game parameter
		) {}

	/// Trigger notification. This method is called when a trigger is added, removed, or changed.
	virtual void OnTriggersNotif( 
		Notif in_eNotif,			///< Notification type
		AkUniqueID in_triggerID		///< Unique ID of the trigger
		) {}

	/// Argument notification. This method is called when an argument or argument value is added, removed, or changed.
	/// \aknote
	/// Although this notification is called when an argument is created, you will probably be more interested to 
	/// know when this argument gets referenced by a dialogue event. See OnDialogueEventNotif().
	/// \endaknote
	virtual void OnArgumentsNotif( 
		Notif in_eNotif,			///< Notification type
		AkUniqueID in_argumentID	///< Unique ID of the trigger
		) {}
			
	/// Axuiliary bus notification. This method is called when an environment is added, removed, or changed.
	virtual void OnAuxBusNotif( 
		Notif in_eNotif,			///< Notification type
		AkUniqueID in_AuxBusID		///< Unique ID of the auxiliary bus
		) {}

	/// Game object notification. This method is called when a game object is registered or unregistered.\n
	/// The notification type will be Notif_Added when a game object is registered, and Notif_Removed 
	/// when its unregistered.
	/// \aknote
	/// - This notification will be sent for game object registration and unregistration made through the Sound Frame 
	/// or the sound engine.
	/// - The notification type will be Notif_Reset when all game objects are removed from the Sound Engine.
	/// \endaknote
	virtual void OnGameObjectsNotif( 
		Notif in_eNotif,				///< Notification type
		AkGameObjectID in_gameObjectID  ///< ID of the game object
		) {}

	AK::SoundFrame::ISoundFrame * GetSoundFrame() { return m_pSoundFrame; }

	UBOOL IsConnected() const;
	AkTimeMs GetCaptureTimeStamp();
#endif

public:
	void RegisterComponent( UAkComponent * in_pComponent );
	void UnregisterComponent( UAkComponent * in_pComponent );

	static UAkAudioDevice * Get();

	void Flush( class FSceneInterface* SceneToFlush );

	void StopGameObject( AkGameObjectID in_GameObjID );

	UAkComponent * GetAkComponent( AActor * in_pActor, FName in_nameBone, bool in_bStopWhenOwnerDestroyed = false );

	void CancelEventCallbackCookie( void* in_cookie );

	UBOOL IsAkComponentValid( UAkComponent* in_pComponent );

	void SetBankDirectory();
	AKRESULT SetBasePath(const FString& Path);

protected:
	bool EnsureInitialized();

	AkPlayingID PostEventInternal(
		const TCHAR * in_pszEvent, 
		AkGameObjectID in_GameObjectID, 
		AkUInt32 in_uFlags /*= 0*/,					///< Bitmask: see \ref AkCallbackType
		AkCallbackFunc in_pfnCallback /*= NULL*/,	///< Callback function
		void * in_pCookie /*= NULL*/				///< Callback cookie that will be sent to the callback function along with additional information.
        );

	// Cleanup.
	void Teardown();

	AKRESULT GetGameObjectID( AActor * in_pActor, FName in_nameBone, AkGameObjectID& io_GameObject, bool in_bStopWhenOwnerDestroyed = false );

	TArray<UAkComponent *> m_GameObjects;
	TMap<FName,IInterface_AkEventHandler*> m_PackageHandlers;

#ifdef AK_SOUNDFRAME
	AK::SoundFrame::ISoundFrame * m_pSoundFrame;
	AK::SoundFrame::CSoundFrameUED *m_pSoundFrameImp;

	AkTimeMs m_TimeStamp;
#endif

	UBOOL bUsingDLCBanks; // rcharpentier - main banks or DLCs banks
	FString ConsoleDLCPath; // path for dynamically mounted DLC (e.g. "/addcont0/")

	static bool m_bSoundEngineInitialized;
#if defined(AK_SOUNDFRAME) && defined(AK_SOUNDFRAME_PLAYBACK)
	static bool m_bUseSoundEngine; // vs. using SoundFrame
#endif
};

#endif