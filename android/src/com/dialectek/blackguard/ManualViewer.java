// Blackguard manual viewer.

package com.dialectek.blackguard;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.widget.TextView;
import android.widget.Toast;

public class ManualViewer extends Activity
{
   @Override
   public void onCreate(Bundle savedInstanceState)
   {
      super.onCreate(savedInstanceState);
      TextView tv = new TextView(this);
      tv.setClickable(false);
      tv.setLongClickable(false);
      tv.setMovementMethod(new ScrollingMovementMethod());
      tv.setHorizontallyScrolling(true);
      tv.setTypeface(Typeface.MONOSPACE);
      tv.setText("Unknown manual path");
      String manualPath = null;
      Bundle extras     = getIntent().getExtras();
      if (extras != null)
      {
         String s = extras.getString("manual_path");
         if (s != null)
         {
            manualPath = s;
            s          = readManual(manualPath);
            if (s != null)
            {
               tv.setText(s);
            }
         }
      }
      setContentView(tv);
   }


   // Get manual text.
   private String readManual(String manualPath)
   {
      ByteArrayOutputStream byteArrayOutputStream = null;

      try
      {
         InputStream inputStream = getAssets().open(manualPath);
         byteArrayOutputStream = new ByteArrayOutputStream(23000);
         int i = inputStream.read();
         while (i != -1)
         {
            byteArrayOutputStream.write(i);
            i = inputStream.read();
         }
         inputStream.close();
      }
      catch (IOException e) {
         Toast.makeText(this,
                        "Cannot read manual " + manualPath,
                        Toast.LENGTH_SHORT).show();
      }
      if (byteArrayOutputStream != null)
      {
         return(byteArrayOutputStream.toString());
      }
      else
      {
         return(null);
      }
   }


   @Override
   public void onConfigurationChanged(Configuration newConfig)
   {
      super.onConfigurationChanged(newConfig);
   }


   @Override
   protected void onStop()
   {
      super.onStop();
      finish();
      int pid = android.os.Process.myPid();
      android.os.Process.killProcess(pid);
   }
}
