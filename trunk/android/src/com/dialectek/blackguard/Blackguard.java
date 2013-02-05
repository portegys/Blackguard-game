/*
 * Blackguard: a 3D rogue-like game.
 *
 * @(#) Blackguard.java	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

package com.dialectek.blackguard;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.UUID;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.speech.RecognizerIntent;
import android.util.Log;

public class Blackguard extends Activity
{
   private static final String LOG_TAG = Blackguard.class .getSimpleName();

   // View.
   private BlackguardView blackguardView;

   // Identity.
   private UUID id;

   // Load native Blackguard.
   static
   {
      try
      {
         System.loadLibrary("Blackguard");
      }
      catch (UnsatisfiedLinkError e)
      {
         Log.w("Blackguard", "Blackguard native library not found in 'java.library.path': " +
               System.getProperty("java.library.path"));
         throw e;
      }
   }

   @Override
   protected void onCreate(Bundle savedInstanceState)
   {
      super.onCreate(savedInstanceState);

      // EULA.
      Eula.show(this);

      // Initialize sounds.
      SoundManager.initSounds(this);
      SoundManager.loadSounds();

      // Establish unique identity.
      setID();

      // Create view.
      blackguardView = new BlackguardView(this, id);
      setContentView(blackguardView);
   }


   // Establish unique identity.
   private void setID()
   {
      id = null;
      String fileName = "id.txt";
      String path     = getDir("data", Context.MODE_PRIVATE).getAbsolutePath() + "/" + fileName;
      try
      {
         BufferedReader in = new BufferedReader(new FileReader(path));
         String         s;
         if ((s = in.readLine()) != null)
         {
            id = UUID.fromString(s);
         }
         in.close();
      }
      catch (FileNotFoundException e) {}
      catch (IOException e)
      {
         Log.w("Blackguard", "Error reading " + fileName);
      }
      if (id == null)
      {
         try
         {
            PrintWriter out = new PrintWriter(new FileWriter(path));
            id = UUID.randomUUID();
            out.println(id.toString());
            out.close();
         }
         catch (IOException e)
         {
            Log.w("Blackguard", "Error writing " + fileName);
         }
      }
   }


   @Override
   public void onConfigurationChanged(Configuration newConfig)
   {
      super.onConfigurationChanged(newConfig);
   }


   @Override
   protected void onPause()
   {
      super.onPause();
      blackguardView.onPause();
   }


   @Override
   protected void onStop()
   {
      super.onStop();
      SoundManager.cleanup();
      blackguardView.onStop();
   }


   @Override
   protected void onResume()
   {
      super.onResume();
      blackguardView.onResume();
   }


   @Override
   public void onDestroy()
   {
      super.onDestroy();
   }


   // Handle the results from the voice recognition activity.
   @Override
   protected void onActivityResult(int requestCode, int resultCode, Intent data)
   {
      if ((requestCode == BlackguardRenderer.VoicePrompter.VOICE_REQUEST_CODE) &&
          (resultCode == RESULT_OK))
      {
         ArrayList<String> matches = data.getStringArrayListExtra(
            RecognizerIntent.EXTRA_RESULTS);
         blackguardView.handleVoiceKeys(matches);
      }
      super.onActivityResult(requestCode, resultCode, data);
   }
}
