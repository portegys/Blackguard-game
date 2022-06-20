// Blackguard view.

package com.dialectek.blackguard;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map.Entry;
import java.util.UUID;

import android.content.ActivityNotFoundException;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.opengl.GLSurfaceView;
import android.os.SystemClock;
import android.speech.RecognizerIntent;
import android.view.GestureDetector;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.Toast;

public class BlackguardView extends GLSurfaceView
implements GestureDetector.OnGestureListener, GestureDetector.OnDoubleTapListener {
   private static final String LOG_TAG = BlackguardView.class.getSimpleName();

   // Context.
   Context context;

   // Renderer.
   BlackguardRenderer renderer;

   // Gesture detection.
   GestureDetector gestures;
   float moveX, moveY;
   static float MOVE_DISTANCE_SCALE = 0.1f;
   static float TURN_DISTANCE_SCALE = 0.2f;
   KeyCharacterMap keyCharacterMap;

   // Voice recognition enabled?
   boolean voiceEnabled;

   // Voice commands and key events.
   ArrayList<String> voiceCommands;
   KeyEvent[] voiceKeyEvents;

   // Viewing manual?
   boolean viewManual;

   // Constructor.
   public BlackguardView(Context context, UUID id) {
      super(context);
      this.context = context;

      // Initialize native Blackguard.
      String[] args = new String[0];
      initBlackguard(args, context.getDir("data",
                      Context.MODE_PRIVATE).getAbsolutePath(),
              id.toString());

      // Create renderer.
      renderer = new BlackguardRenderer(context, this);
      setRenderer(renderer);
      renderer.modeTimer = SystemClock.uptimeMillis();
      setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

      // Load virtual keyboard map.
      keyCharacterMap = KeyCharacterMap.load(-1);

      // Create gesture detector.
      moveX = moveY = 0.0f;
      gestures = new GestureDetector(context, this);
      gestures.setOnDoubleTapListener(this);

      // Set voice recognition status.
      PackageManager pm = context.getPackageManager();
      List<ResolveInfo> activities = pm.queryIntentActivities(
              new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH), 0);
      if (activities.size() == 0) {
         voiceEnabled = false;
      } else {
         voiceEnabled = true;
         initVoiceRecognition();
      }

      // Not viewing manual.
      viewManual = false;
   }


   // Key press events.
   @Override
   public boolean onKeyDown(int keyCode, KeyEvent event) {
      switch (keyCode) {
         case KeyEvent.KEYCODE_CALL:
         case KeyEvent.KEYCODE_CAMERA:
         case KeyEvent.KEYCODE_ENDCALL:
         case KeyEvent.KEYCODE_ENVELOPE:
         case KeyEvent.KEYCODE_EXPLORER:
         case KeyEvent.KEYCODE_FOCUS:
         case KeyEvent.KEYCODE_HOME:
         case KeyEvent.KEYCODE_HEADSETHOOK:
         case KeyEvent.KEYCODE_MENU:
         case KeyEvent.KEYCODE_POWER:
         case KeyEvent.KEYCODE_VOLUME_DOWN:
         case KeyEvent.KEYCODE_VOLUME_UP:
            return (super.onKeyDown(keyCode, event));
      }

      // View manual?
      if (keyCode == KeyEvent.KEYCODE_SEARCH) {
         return (viewManual());
      }

      if (renderer.view == BlackguardRenderer.View.OVERVIEW) {
         if (keyCode == KeyEvent.KEYCODE_BACK) {
            renderer.view = BlackguardRenderer.View.FPVIEW;
            pokeRenderer();
         }
         return (true);
      }

      char keyChar = (char) event.getUnicodeChar();

      switch (keyCode) {
         case KeyEvent.KEYCODE_SHIFT_LEFT:
         case KeyEvent.KEYCODE_SHIFT_RIGHT:
            return (true);

         case KeyEvent.KEYCODE_BACK:
            // Escape.
            keyChar = (char) 27;
            break;

         case KeyEvent.KEYCODE_DPAD_UP:
            keyChar = 'K';
            break;

         case KeyEvent.KEYCODE_DPAD_DOWN:
            keyChar = 'J';
            break;

         case KeyEvent.KEYCODE_DPAD_RIGHT:
            int d = playerDir();
            d--;
            if (d < 0) {
               d += 8;
            }
            if (event.isShiftPressed()) {
               d--;
               if (d < 0) {
                  d += 8;
               }
            }
            playerDir(d);
            pokeRenderer();
            return (true);

         case KeyEvent.KEYCODE_DPAD_LEFT:
            d = playerDir();
            d = (d + 1) % 8;
            if (event.isShiftPressed()) {
               d = (d + 1) % 8;
            }
            playerDir(d);
            pokeRenderer();
            return (true);
      }

      // Clear prompt for space signal.
      spacePrompt();

      // Wait for input request?
      lockInput();
      if (inputReq() == 0) {
         waitInput();
      }

      switch (inputReq()) {
         // Request for character.
         case 1:
            switch (keyCode) {
               case KeyEvent.KEYCODE_ENTER:
                  inputChar('\n');
                  break;

               case KeyEvent.KEYCODE_DEL:
                  inputChar('\b');
                  break;

               default:
                  if (!event.isShiftPressed()) {
                     keyChar = Character.toLowerCase(keyChar);
                  } else {
                     // Map gt/lt keys.
                     if (keyChar == ';') {
                        keyChar = '<';
                     } else if (keyChar == ':') {
                        keyChar = '>';
                     }
                  }
                  inputChar(keyChar);
                  break;
            }
            inputReq(0);
            signalInput();
            break;

         // Request for string.
         case 2:
            switch (keyCode) {
               case KeyEvent.KEYCODE_ENTER:
                  inputReq(0);
                  signalInput();
                  break;

               case KeyEvent.KEYCODE_DEL:
                  if (inputIndex() > 0) {
                     inputIndex(inputIndex() - 1);
                     inputBuf(inputIndex(), '\0');
                  }
                  break;

               default:
                  if (inputIndex() < inputSize() - 1) {
                     if (!event.isShiftPressed()) {
                        keyChar = Character.toLowerCase(keyChar);
                     } else {
                        // Map gt/lt keys.
                        if (keyChar == ';') {
                           keyChar = '<';
                        } else if (keyChar == ':') {
                           keyChar = '>';
                        }
                     }
                     inputBuf(inputIndex(), keyChar);
                     inputIndex(inputIndex() + 1);
                     inputBuf(inputIndex(), '\0');
                  }
                  break;
            }
            break;

         default:
            break;
      }
      unlockInput();
      pokeRenderer();
      return (true);
   }

   // View game manual.
   boolean viewManual()
   {
      String manualPath = "doc/blackguard.txt";
      Intent intent     = new Intent(Intent.ACTION_VIEW);

      intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
      intent.putExtra("manual_path", manualPath);
      intent.setClassName("com.dialectek.blackguard", "com.dialectek.blackguard.ManualViewer");
      viewManual = true;
      ((Blackguard)context).intentActive = true;
      try {
         context.startActivity(intent);
      }
      catch (ActivityNotFoundException e) {
         viewManual = false;
         ((Blackguard)context).intentActive = false;
         Toast.makeText(context,
                        "No activity available to view " + manualPath,
                        Toast.LENGTH_SHORT).show();
      }
      return(true);
   }


   // Initialize voice recognition.
   void initVoiceRecognition()
   {
      // Build command/key table.
      // Prepend "shift" keyword for uppercase keys.
      ContentValues cmdKeys = new ContentValues();

      cmdKeys.put("throw", "t");
      cmdKeys.put("zap", "z");
      cmdKeys.put("dip", "shift d");
      cmdKeys.put("down", "shift .");
      cmdKeys.put("up", "shift ,");
      cmdKeys.put("search", "s");
      cmdKeys.put("inventory", "i");
      cmdKeys.put("pack", "i");
      cmdKeys.put("quaff", "q");
      cmdKeys.put("drink", "q");
      cmdKeys.put("read", "r");
      cmdKeys.put("eat", "e");
      cmdKeys.put("wield", "w");
      cmdKeys.put("wear", "shift w");
      cmdKeys.put("take", "shift t");
      cmdKeys.put("put", "shift p");
      cmdKeys.put("remove", "shift r");
      cmdKeys.put("drop", "d");
      cmdKeys.put("encumbrance", "a");
      cmdKeys.put("weight", "a");
      cmdKeys.put("version", "v");
      cmdKeys.put("show", "shift s");
      cmdKeys.put("mute", "shift m");
      cmdKeys.put("identity", "@");
      cmdKeys.put("quit", "shift q");
      cmdKeys.put("star", "shift 8");
      cmdKeys.put("list", "shift 8");
      cmdKeys.put("left", "l");
      cmdKeys.put("right", "r");
      cmdKeys.put("manual", "m");
      cmdKeys.put("help", "shift /");
      cmdKeys.put("return", "enter");
      cmdKeys.put("enter", "enter");
      cmdKeys.put("yes", "y");
      cmdKeys.put("no", "n");
      cmdKeys.put("conjure", "shift `");
      cmdKeys.put("potion", "shift 1");
      cmdKeys.put("scroll", "shift /");
      cmdKeys.put("food", "shift ;");

      // Create commands.
      voiceCommands = new ArrayList<String>();
      for (Entry<String, Object> entry : cmdKeys.valueSet())
      {
         voiceCommands.add(entry.getKey());
      }
      Collections.sort(voiceCommands);

      // Create key codes.
      voiceKeyEvents = new KeyEvent[cmdKeys.size()];
      for (Entry<String, Object> entry : cmdKeys.valueSet())
      {
         voiceKeyEvents[Collections.binarySearch(voiceCommands, entry.getKey())] =
            getKeyEvent(entry.getValue().toString());
      }
   }

   // Get key event for string.
   KeyEvent getKeyEvent(String keyStr)
   {
      String[] keywords = keyStr.split(" ");
      String  keyword;
      boolean shift = false;
      if (keywords.length == 1)
      {
         keyword = keywords[0];
      }
      else if ((keywords.length == 2) && keywords[0].equals("shift"))
      {
         keyword = keywords[1];
         shift   = true;
      }
      else
      {
         return(null);
      }
      if (keyword.length() > 0)
      {
         if (keyword.equals("enter"))
         {
            return(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER));
         }
         else
         {
            char[] keyChars      = keyword.toCharArray();
            KeyEvent[] keyevents = keyCharacterMap.getEvents(keyChars);
            if (keyevents.length > 0)
            {
               if (shift)
               {
                  return(new KeyEvent(0, 0, KeyEvent.ACTION_DOWN, keyevents[0].getKeyCode(), 0, KeyEvent.META_SHIFT_ON));
               }
               else
               {
                  return(new KeyEvent(KeyEvent.ACTION_DOWN, keyevents[0].getKeyCode()));
               }
            }
         }
      }
      return(null);
   }


   // Handle voice keys.
   void handleVoiceKeys(ArrayList<String> matches)
   {
      int i, j;

      String[] words = null;
      KeyEvent keyevent;

      // Handle command.
      for (i = j = 0; i < matches.size(); i++)
      {
         words = matches.get(i).split(" ");
         if (words.length > 0)
         {
            if ((words.length == 2) && words[0].equals("search"))
            {
               handleNumSearches(words[1]);
               words    = new String[1];
               words[0] = "search";
            }
            j = Collections.binarySearch(voiceCommands, words[0]);
            if (j >= 0)
            {
               keyevent = voiceKeyEvents[j];
               if ((words.length >= 2) &&
                   (words[0].equals("inventory") || words[0].equals("pack")))
               {
                  keyevent = new KeyEvent(0, 0, KeyEvent.ACTION_DOWN, keyevent.getKeyCode(), 0, KeyEvent.META_SHIFT_ON);
               }
               onKeyDown(keyevent.getKeyCode(), keyevent);
               j = 1;
               if (words.length > 1)
               {
                  if (words[0].equals("remove"))
                  {
                     if (words[1].equals("left"))
                     {
                        keyevent = getKeyEvent("left");
                        onKeyDown(keyevent.getKeyCode(), keyevent);
                        j++;
                     }
                     if (words[1].equals("right"))
                     {
                        keyevent = getKeyEvent("right");
                        onKeyDown(keyevent.getKeyCode(), keyevent);
                        j++;
                     }
                  }
                  if (words[0].equals("take") && words[1].equals("off")) { j++; }
                  if (words[0].equals("put") && words[1].equals("on")) { j++; }
               }
               break;
            }
         }
      }
      if (i >= matches.size())
      {
         if (matches.size() == 0) { return; }
         i     = 0;
         words = matches.get(i).split(" ");
         if (words.length == 0) { return; }
         j = 0;
      }
      else
      {
         if (j >= words.length) { return; }
      }

      // Input characters.
      boolean shift = false;
      for ( ; j < words.length; j++)
      {
         if (words[j].equals("return") || words[j].equals("enter"))
         {
            keyevent = getKeyEvent("enter");
            onKeyDown(keyevent.getKeyCode(), keyevent);
            shift = false;
         }
         else if (words[j].equals("star") || words[j].equals("list"))
         {
            keyevent = getKeyEvent("list");
            onKeyDown(keyevent.getKeyCode(), keyevent);
            shift = false;
         }
         else if (words[j].equals("shift") || words[j].equals("uppercase") ||
                  words[j].equals("capital") || words[j].equals("capitol"))
         {
            shift = true;
         }
         else
         {
            if (words[j].equals("Apple")) words[j] = "apple";
            char[] keyChars = words[j].toCharArray();

            if (keyChars.length > 0)
            {
               char[] c             = new char[1];
               c[0]                 = keyChars[0];
               KeyEvent[] keyevents = keyCharacterMap.getEvents(c);
               if (keyevents.length > 0)
               {
                  if (shift)
                  {
                     keyevent = new KeyEvent(0, 0, KeyEvent.ACTION_DOWN, keyevents[0].getKeyCode(), 0, KeyEvent.META_SHIFT_ON);
                  }
                  else
                  {
                     keyevent = new KeyEvent(KeyEvent.ACTION_DOWN, keyevents[0].getKeyCode());
                  }
                  onKeyDown(keyevent.getKeyCode(), keyevent);
               }
            }
            shift = false;
         }
      }
   }


   // Handle number of searches.
   void handleNumSearches(String num)
   {
      if (num.equals("five"))
      {
         num = "5";
      }
      else if (num.equals("ten"))
      {
         num = "10";
      }
      else if (num.equals("twenty"))
      {
         num = "20";
      }
      else if (num.equals("thiry"))
      {
         num = "30";
      }
      else if (num.equals("forty"))
      {
         num = "40";
      }
      else if (num.equals("fify"))
      {
         num = "50";
      }
      else
      {
         return;
      }

      KeyEvent keyevent;
      char[] keyChars = num.toCharArray();
      char[] c        = new char[1];
      for (int k = 0; k < keyChars.length; k++)
      {
         c[0] = keyChars[k];
         KeyEvent[] keyevents = keyCharacterMap.getEvents(c);
         if (keyevents.length > 0)
         {
            keyevent = new KeyEvent(KeyEvent.ACTION_DOWN, keyevents[0].getKeyCode());
            onKeyDown(keyevent.getKeyCode(), keyevent);
         }
      }
   }


   // Touch event.
   @Override
   public boolean onTouchEvent(final MotionEvent event)
   {
      // Process gestures.
      gestures.onTouchEvent(event);

      // Detect completion of gesture.
      if (event.getAction() == MotionEvent.ACTION_UP)
      {
         moveX = moveY = 0.0f;
      }
      return(true);
   }


   // Single tap.
   public boolean onSingleTapConfirmed(MotionEvent event)
   {
      int keyCode, keyChar;

      if (renderer.view == BlackguardRenderer.View.OVERVIEW)
      {
         renderer.view = BlackguardRenderer.View.FPVIEW;
         pokeRenderer();
         return(true);
      }

      // Soft keyboard toggled?
      if (renderer.softKeyboardToggle.touched((int)event.getX(), (int)event.getY()))
      {
         renderer.softKeyboardToggle.toggle();
         pokeRenderer();
         return(true);
      }

      // Prompt for voice command?
      if (voiceEnabled && renderer.voicePrompterVisible &&
          renderer.voicePrompter.touched((int)event.getX(), (int)event.getY()))
      {
         renderer.voicePrompter.prompt();
         pokeRenderer();
         return(true);
      }

      if (spacePrompt())
      {
         // Shortcut for space key.
         keyCode = KeyEvent.KEYCODE_SPACE;
         KeyEvent keyevent = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
         return(onKeyDown(keyCode, keyevent));
      }

      if ((keyChar = renderer.pickCharacter((int)event.getX(), (int)event.getY())) != 0)
      {
         // Identify character.
         KeyEvent keyevent = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_SLASH);
         onKeyDown(KeyEvent.KEYCODE_SLASH, keyevent);
         lockInput();
         if (inputReq() == 0)
         {
            waitInput();
         }
         inputChar((char)keyChar);
         inputReq(0);
         signalInput();
         unlockInput();
         pokeRenderer();
         return(true);
      }
      return(true);
   }


   // Double tap.
   public boolean onDoubleTap(MotionEvent event)
   {
      if ((renderer.view == BlackguardRenderer.View.FPVIEW))
      {
         if (renderer.promptPresent)
         {
            // This is a fallback when there is no back button.
            int      keyCode  = KeyEvent.KEYCODE_BACK;
            KeyEvent keyevent = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
            return(onKeyDown(keyCode, keyevent));
         }
         else
         {
            renderer.view = BlackguardRenderer.View.OVERVIEW;
            pokeRenderer();
         }
      }
      else
      {
         renderer.view = BlackguardRenderer.View.FPVIEW;
         pokeRenderer();
      }
      return(true);
   }


   // Scroll.
   public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
   {
      int      keyCode;
      KeyEvent keyEvent;

      moveX += distanceX;
      moveY += distanceY;

      float moveDistance = 0.0f;
      float turnDistance = 0.0f;
      if (renderer.windowWidth < renderer.windowHeight)
      {
         moveDistance = renderer.windowWidth * MOVE_DISTANCE_SCALE;
         turnDistance = renderer.windowWidth * TURN_DISTANCE_SCALE;
      }
      else
      {
         moveDistance = renderer.windowHeight * MOVE_DISTANCE_SCALE;
         turnDistance = renderer.windowHeight * TURN_DISTANCE_SCALE;
      }
      while (Math.abs(moveX) >= turnDistance || Math.abs(moveY) > moveDistance)
      {
         if (moveY >= moveDistance)
         {
            moveY   -= moveDistance;
            keyCode  = KeyEvent.KEYCODE_K;
            keyEvent = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
            onKeyDown(keyCode, keyEvent);
         }
         else if (moveY <= -moveDistance)
         {
            moveY   += moveDistance;
            keyCode  = KeyEvent.KEYCODE_J;
            keyEvent = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
            onKeyDown(keyCode, keyEvent);
         }

         if (moveX >= turnDistance)
         {
            moveX   -= turnDistance;
            keyCode  = KeyEvent.KEYCODE_DPAD_LEFT;
            keyEvent = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
            onKeyDown(keyCode, keyEvent);
         }
         else if (moveX <= -turnDistance)
         {
            moveX   += turnDistance;
            keyCode  = KeyEvent.KEYCODE_DPAD_RIGHT;
            keyEvent = new KeyEvent(KeyEvent.ACTION_DOWN, keyCode);
            onKeyDown(keyCode, keyEvent);
         }
      }
      return(true);
   }


   // Fling.
   public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY)
   {
      return(true);
   }


   public void onLongPress(MotionEvent event)
   {
   }


   public void onShowPress(MotionEvent event)
   {
   }


   public boolean onSingleTapUp(MotionEvent event)
   {
      return(true);
   }


   public boolean onDoubleTapEvent(MotionEvent event)
   {
      return(true);
   }


   public boolean onDown(MotionEvent event)
   {
      return(true);
   }


   @Override
   public void onResume()
   {
      super.onResume();

      // Process will die after viewing manual.
      if (!viewManual)
      {
         // Set focus.
         setFocusable(true);
         setFocusableInTouchMode(true);
         requestFocus();

         // Activate renderer.
         renderer.invalidateTextures();
         pokeRenderer();
      }
   }

   @Override
   public void onPause()
   {
      // Detect intent termination.
      if (((Blackguard)context).intentActive)
      {
         ((Blackguard)context).intentActive = false;
         return;
      }

      super.onPause();

      // Wait for game thread to block on input.
      lockInput();
      if (inputReq() == 0)
      {
         waitInput();
      }
      unlockInput();

      // Save for later resumption.
      saveBlackguard();
   }


   // Stop.
   public void onStop()
   {
      // If user re-launches a new activity, it will clash with this one,
      // possibly because of native shared library and thread?
      // Therefore it is best to exit and resume by loading the save file.
      // Note1: Tried finish() and android:finishOnTaskLaunch without success.
      // Note2: If manual viewer is up, let it exit.
      if (!viewManual)
      {
         int pid = android.os.Process.myPid();
         android.os.Process.killProcess(pid);
      }
   }


   // Activate renderer.
   void pokeRenderer()
   {
      renderer.modeTimer = SystemClock.uptimeMillis();
      setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
      requestRender();
   }


   // Native methods:

   // Initialize Blackguard.
   private native void initBlackguard(String[] args, String localDir, String id);

   // Save.
   private native void saveBlackguard();

   // Delete.
   private native void deleteBlackguard();

   // Get/set player direction.
   private native int playerDir();
   private native int playerDir(int dir);

   // Input synchronization.
   private native void lockInput();
   private native void unlockInput();
   private native void waitInput();
   private native void signalInput();
   private native int inputReq();
   private native int inputReq(int state);
   private native boolean spacePrompt();

   // Input.
   private native void inputChar(char c);
   private native int inputIndex();
   private native int inputIndex(int i);
   private native void inputBuf(int i, char c);
   private native int inputSize();
}
