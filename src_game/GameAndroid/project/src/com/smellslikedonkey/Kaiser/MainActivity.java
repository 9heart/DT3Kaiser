package com.smellslikedonkey.Kaiser;

import android.app.NativeActivity;
import android.app.PendingIntent;
import android.widget.PopupWindow;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.view.ViewGroup.MarginLayoutParams;
import android.graphics.drawable.ColorDrawable;
import android.view.Gravity;
import android.os.Bundle;
import android.os.IBinder;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.content.Context;
import android.util.Log;
import org.json.JSONObject;

import java.util.ArrayList;
import com.google.ads.*;
import com.android.vending.billing.*;

public class MainActivity extends NativeActivity
{
    AdView          _ad_view;
    PopupWindow     _pop_up;

    MainActivity    _activity;
    LinearLayout    _main_layout;
    
    
    // Service Connection
    IInAppBillingService mService;

    ServiceConnection mServiceConn = new ServiceConnection() {
       @Override
       public void onServiceDisconnected(ComponentName name) {
           mService = null;
       }

       @Override
       public void onServiceConnected(ComponentName name, 
          IBinder service) {
           mService = IInAppBillingService.Stub.asInterface(service);
       }
    };

    private static final String TAG = "MainActivity";

    //
    // Lifetime
    //

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        _activity = this;

        // Create our ad view here
        _ad_view = new AdView(_activity, AdSize.BANNER, "CHANGEME");
        
        // Bind IAP service
        bindService(new Intent("com.android.vending.billing.InAppBillingService.BIND"), mServiceConn, Context.BIND_AUTO_CREATE);
    }
    
    // Do some cleanup
    @Override
    public void onDestroy() {
        if (_ad_view != null) {
            _ad_view.destroy();
        }

        if (mService != null) {
            unbindService(mServiceConn);
        }   

        super.onDestroy();
    }


    //
    // Ads
    //

    // Our popup window, you will call it from your C/C++ code later
    public void showAdPopup()
    {
        Log.v(TAG, "showAdPopup called");

        if (_ad_view != null)  {
            _activity.runOnUiThread(new Runnable()  {
                @Override
                public void run()  {
                
                    if (_pop_up == null) {
                        // Our popup window
                        _pop_up = new PopupWindow(_activity);

                        // This is the minimum size for AdMob, we need to set this in case our target device run at 320x480 resolution (Otherwise no ad will be shown, see the padding kill below)
                        _pop_up.setWidth(320);
                        _pop_up.setHeight(50);
                    
                        _pop_up.setBackgroundDrawable(new ColorDrawable(android.graphics.Color.TRANSPARENT));

                        _pop_up.setWindowLayoutMode(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
                        _pop_up.setClippingEnabled(false);

                        LinearLayout layout = new LinearLayout(_activity);

                        // The layout system for the PopupWindow will kill some pixels due to margins/paddings etc… (No way to remove it), so padd it to adjust
                        layout.setPadding(-5, -5, -5, -5);

                        MarginLayoutParams params = new MarginLayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
                        params.setMargins(0, 0, 0, 0);

                        layout.setOrientation(LinearLayout.VERTICAL);
                        layout.addView(_ad_view, params);

                        _pop_up.setContentView(layout);

                        _main_layout = new LinearLayout(_activity);
                        _activity.setContentView(_main_layout, params);

                        AdRequest adRequest = new AdRequest();

                        // Enable this if your are testing AdMob, otherwise you'll risk to be banned!
                        //adRequest.addTestDevice(AdRequest.TEST_EMULATOR);

                        _activity._ad_view.loadAd(adRequest);

                    }
                    
                    // Show our popup window
                    _pop_up.showAtLocation(_main_layout, Gravity.TOP, 0, 0);
                    _pop_up.update();

                } // run
            
            });
        }
        
    }
    
    public void hideAdPopup() {
        Log.v(TAG, "hideAdPopup called");

        if (_ad_view != null)  {
            _activity.runOnUiThread(new Runnable()  {
                @Override
                public void run()  {
                
                    if (_pop_up != null && _pop_up.isShowing()) {
                        _pop_up.dismiss();
                    }
                    
                } // run
            
            });
            
        }
        
    }


    //
    // IAP
    //
    
    public static class InAppPurchasesProduct {        
        String  localized_description;
        String  localized_title;
        String  price;
        String  product_identifier;
    };
    
    native void requestProductsInfoCB (ArrayList<InAppPurchasesProduct> products, int error, long fn_opaque, long data_opaque);
    native void restoreProductCB (String product, int error, long fn_opaque, long data_opaque);
    native void purchaseProductCB (String product, int error, long fn_opaque, long data_opaque);

    
    int requestProductsInfo (ArrayList<String> products, long fn_opaque, long data_opaque)
    {
        Log.v(TAG, "requestProductsInfo called");
        for (String p : products) {
            Log.v(TAG, " Product: " + p);
        }
        
        Bundle query_skus = new Bundle();
        query_skus.putStringArrayList("ITEM_ID_LIST", products);
        
        // NOTE: This blocks
        try {
            Bundle sku_details = mService.getSkuDetails(3, getPackageName(), "inapp", query_skus);
        
            // How to get info
            int response = sku_details.getInt("RESPONSE_CODE");
            Log.v(TAG, "    response " + response);

            if (response == 0) {
                ArrayList<String> responseList = sku_details.getStringArrayList("DETAILS_LIST");
                Log.v(TAG, "    responseList " + responseList.size());

                ArrayList<InAppPurchasesProduct> returned_products = new ArrayList<InAppPurchasesProduct>();

                for (String thisResponse : responseList) {
                    try {
                        JSONObject object = new JSONObject(thisResponse);

                        InAppPurchasesProduct product = new InAppPurchasesProduct();
                        product.localized_description = object.getString("description");
                        product.localized_title = object.getString("title");
                        product.price = object.getString("price");
                        product.product_identifier = object.getString("productId");
                        returned_products.add(product);
                    }
                    catch(org.json.JSONException e) {
                    }
                }
            
                requestProductsInfoCB (returned_products, 0, fn_opaque, data_opaque);
            }
        }
        catch (android.os.RemoteException e) {
            e.printStackTrace();
        }

        return 0;
    }

    int restoreProducts (long fn_opaque, long data_opaque)
    {
        Log.v(TAG, "restoreProducts called");

        try {
            Bundle ownedItems = mService.getPurchases(3, getPackageName(), "inapp", null);

            while (true) {
                int response = ownedItems.getInt("RESPONSE_CODE");
                if (response != 0)
                    break;

                ArrayList<String> ownedSkus = ownedItems.getStringArrayList("INAPP_PURCHASE_ITEM_LIST");
                ArrayList<String> purchaseDataList = ownedItems.getStringArrayList("INAPP_PURCHASE_DATA_LIST");
                ArrayList<String> signatureList = ownedItems.getStringArrayList("INAPP_DATA_SIGNATURE");
                String continuationToken = ownedItems.getString("INAPP_CONTINUATION_TOKEN");

                for (int i = 0; i < purchaseDataList.size(); ++i) {
                    //String purchaseData = purchaseDataList.get(i);
                    //String signature = signatureList.get(i);
                    String sku = ownedSkus.get(i);

                    restoreProductCB (sku, 0, fn_opaque, data_opaque);
                } 

                // if continuationToken != null, call getPurchases again 
                // and pass in the token to retrieve more items

                if (continuationToken != null) {
                    ownedItems = mService.getPurchases(3, getPackageName(), "inapp", continuationToken);
                    continue;
                }
            
                break;
            }
            
        }
        catch (android.os.RemoteException e) {
            e.printStackTrace();
        }

        return 0;
    }

    int makePurchase (InAppPurchasesProduct product, long fn_opaque, long data_opaque)
    {
        Log.v(TAG, "makePurchase called " + product.product_identifier);

        try {
        
            Bundle buyIntentBundle = mService.getBuyIntent(
                3, 
                getPackageName(), 
                product.product_identifier, 
                "inapp", 
                "dummystring"
            );
            
            // Check if any items need consuming
            int response = buyIntentBundle.getInt("RESPONSE_CODE");
            if (response == 7) {
                purchaseProductCB (product.product_identifier, 0, fn_opaque, data_opaque);
                return 0;
            }

            PendingIntent pendingIntent = buyIntentBundle.getParcelable("BUY_INTENT");
            if (pendingIntent == null)
                Log.v(TAG, "pendingIntent is null!!");
            
            Intent intent = new Intent();
            intent.putExtra("fn_opaque",fn_opaque);
            intent.putExtra("data_opaque",data_opaque);

            startIntentSenderForResult(
                pendingIntent.getIntentSender(),
                1001, 
                intent, 
                Integer.valueOf(0), 
                Integer.valueOf(0),
                Integer.valueOf(0)
            );

        } 
        catch (android.os.RemoteException e) {
            e.printStackTrace();
        }
        catch (android.content.IntentSender.SendIntentException e) {
            e.printStackTrace();
        }

        return 0;
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.v(TAG, "onActivityResult called");
 
        if (requestCode == 1001) {           
            int responseCode = data.getIntExtra("RESPONSE_CODE", 0);
            String purchaseData = data.getStringExtra("INAPP_PURCHASE_DATA");
            String dataSignature = data.getStringExtra("INAPP_DATA_SIGNATURE");

            if (resultCode == RESULT_OK) {
                try {
                    JSONObject jo = new JSONObject(purchaseData);
                    String sku = jo.getString("productId");
                    
                    long fn_opaque = data.getExtras().getLong("fn_opaque");
                    long data_opaque = data.getExtras().getLong("data_opaque");
                    
                    purchaseProductCB (sku, 0, fn_opaque, data_opaque);
                }
                catch (org.json.JSONException e) {
                    e.printStackTrace();
                }
            }
        }
    }




    static {
       System.loadLibrary("GameAndroid");
    }
 }

