// Sound manager.

package com.dialectek.blackguard;

import java.io.IOException;
import java.util.HashMap;
import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.SoundPool;
import android.util.Log;

public class SoundManager
{
   // Sounds.
   // See also Blackgaurd.cpp
   public static final int openDungeonSound  = 0;
   public static final int pickupGoldSound   = 1;
   public static final int pickupObjectSound = 2;
   public static final int hitMonsterSound   = 3;
   public static final int hitPlayerSound    = 4;
   public static final int killMonsterSound  = 5;
   public static final int levelUpSound      = 6;
   public static final int throwObjectSound  = 7;
   public static final int thunkMissileSound = 8;
   public static final int zapWandSound      = 9;
   public static final int dipObjectSound    = 10;
   public static final int eatFoodSound      = 11;
   public static final int quaffPotionSound  = 12;
   public static final int readScrollSound   = 13;
   public static final int stairsSound       = 14;
   public static final int dieSound          = 15;
   public static final int winnerSound       = 16;

   private static SoundPool                 mSoundPool;
   private static HashMap<Integer, Integer> mSoundPoolMap;
   private static AudioManager              mAudioManager;
   private static Context mContext;

   private SoundManager()
   {
   }


   /**
    * Initializes the storage for the sounds
    *
    * @param theContext The Application context
    */
   public static void initSounds(Context theContext)
   {
      mContext      = theContext;
      mSoundPool    = new SoundPool(4, AudioManager.STREAM_MUSIC, 0);
      mSoundPoolMap = new HashMap<Integer, Integer>();
      mAudioManager = (AudioManager)mContext.getSystemService(Context.AUDIO_SERVICE);
   }


   /**
    * Add a new Sound to the SoundPool
    *
    * @param Index - The Sound Index for Retrieval
    * @param SoundID - The Android ID for the Sound asset.
    */
   public static void addSound(int Index, int SoundID)
   {
      mSoundPoolMap.put(Index, mSoundPool.load(mContext, SoundID, 1));
   }


   /**
    * Loads the various sound assets
    */
   public static void loadSounds()
   {
      loadSound("opendungeon.ogg", openDungeonSound);
      loadSound("pickupgold.ogg", pickupGoldSound);
      loadSound("pickupobject.ogg", pickupObjectSound);
      loadSound("hitmonster.ogg", hitMonsterSound);
      loadSound("hitplayer.ogg", hitPlayerSound);
      loadSound("killmonster.ogg", killMonsterSound);
      loadSound("experienceup.ogg", levelUpSound);
      loadSound("throwobject.ogg", throwObjectSound);
      loadSound("thunkmissile.ogg", thunkMissileSound);
      loadSound("zapwand.ogg", zapWandSound);
      loadSound("dipobject.ogg", dipObjectSound);
      loadSound("eatfood.ogg", eatFoodSound);
      loadSound("quaffpotion.ogg", quaffPotionSound);
      loadSound("readscroll.ogg", readScrollSound);
      loadSound("gostairs.ogg", stairsSound);
      loadSound("die.ogg", dieSound);
      loadSound("winner.ogg", winnerSound);
   }


   /**
    * Load a sound
    *
    * @param fileName - The Sound asset file name.
    * @param Index - The Sound Index for Retrieval
    */
   public static void loadSound(String fileName, int Index)
   {
      AssetFileDescriptor assetFd = null;

      try
      {
         assetFd = mContext.getAssets().openFd("sounds/" + fileName);
         mSoundPoolMap.put(Index, mSoundPool.load(assetFd, 1));
      }
      catch (IOException e)
      {
         Log.w("Blackguard", "Cannot open sound file: " + fileName);
      }
   }


   /**
    * Plays a Sound
    *
    * @param index - The Index of the Sound to be played
    * @param speed - The Speed to play not, not currently used but included for compatibility
    */
   public static void playSound(int index, float speed)
   {
      if (mute()) { return; }

      float streamVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);

      streamVolume = streamVolume / mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
      Integer id = mSoundPoolMap.get(index);
      if (id != null)
      {
         mSoundPool.play(id, streamVolume, streamVolume, 1, 0, speed);
      }
   }


   /**
    * Stop a Sound
    * @param index - index of the sound to be stopped
    */
   public static void stopSound(int index)
   {
      mSoundPool.stop(mSoundPoolMap.get(index));
   }


   public static void cleanup()
   {
      mSoundPool.release();
      mSoundPool = null;
      mSoundPoolMap.clear();
      mAudioManager.unloadSoundEffects();
   }


   // Mute.
   private static native boolean mute();
}
