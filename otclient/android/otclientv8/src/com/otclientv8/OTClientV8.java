package com.otclientv8;

import android.app.Activity;
import android.app.AlertDialog;
import android.view.View;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.EditText;
import android.content.Context;
import android.content.DialogInterface;
import android.text.InputType;
import com.otclientv8.R;
import android.view.inputmethod.InputMethodManager;
import java.util.concurrent.Semaphore;
import android.content.Intent;
import android.net.Uri;

public class OTClientV8 extends android.app.NativeActivity
{
    static {
        System.loadLibrary("otclientv8");
    }

    public void showKeyboard()
    {
        InputMethodManager imm = ( InputMethodManager )getSystemService( Context.INPUT_METHOD_SERVICE );
        imm.showSoftInput( this.getWindow().getDecorView(), InputMethodManager.SHOW_FORCED );
    }

    public void hideKeyboard()
    {
        InputMethodManager imm = ( InputMethodManager )getSystemService( Context.INPUT_METHOD_SERVICE );
        imm.hideSoftInputFromWindow( this.getWindow().getDecorView().getWindowToken(), 0 );
    }

    private final Semaphore semaphore = new Semaphore(0, true);
    public void displayFatalError(final String message)
    {
        final OTClientV8 activity = this;
        this.runOnUiThread(new Runnable() {
           public void run() {
               AlertDialog.Builder builder = new AlertDialog.Builder(activity, AlertDialog.THEME_HOLO_DARK);
               builder.setTitle("OTClientV8 Fatal Error");
               builder.setMessage(message);
               builder.setPositiveButton("Close", new DialogInterface.OnClickListener() {
                   public void onClick(DialogInterface dialog, int id) {
                        semaphore.release();
                   }
               });
               builder.setCancelable(false);
               AlertDialog dialog = builder.create();
               dialog.show();
           }
        });
        try {
            semaphore.acquire();
        }
        catch (InterruptedException e) { }
    }

    public void showTextEdit(final String title, final String description, final String text, final int flags)
    {
    	if(activeDialog != null) {
            activeDialog.dismiss();
            activeDialog = null;
    	}

        final OTClientV8 activity = this;
        this.runOnUiThread(new Runnable() {
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(activity);
                builder.setTitle(title);
                int style = R.layout.editor;
                if((flags & 0x01) > 0) {
                    style = R.layout.multiline_editor;
                }
                View viewInflated = LayoutInflater.from(activity).inflate(style, (ViewGroup) findViewById(android.R.id.content), false);
                final EditText input = (EditText) viewInflated.findViewById(R.id.input);
                // Specify the type of input expected; this, for example, sets the input as a password, and will mask the text
                input.setText(text);
                input.setSelection(text.length());
                if(!description.isEmpty())
                    builder.setMessage(description);
                builder.setView(viewInflated);

                // Set up the buttons
                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() { 
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        commitText(input.getText().toString());
                        if(dialog == activeDialog)
                            activeDialog = null;
                    }
                });
                builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.cancel();
                        if(dialog == activeDialog)
                            activeDialog = null;
                    }
                });
                activeDialog = builder.show();
                /*
                // Keyboard
                final InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);

                // Auto show keyboard
                input.setOnFocusChangeListener(new View.OnFocusChangeListener() {
                    @Override
                    public void onFocusChange(View v, boolean isFocused) {

                        if (isFocused) {
                            imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
                        } 
                    }
                }); */
            }
        });
    }

    public void openUrl(String url) 
    {
        if (!url.contains("://"))
           url = "http://" + url;
        Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        startActivity(browserIntent);
    }

    private AlertDialog activeDialog = null;
    public static native void commitText(String text);
}
