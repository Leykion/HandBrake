/* $Id: PictureController.h,v 1.6 2005/04/14 20:40:05 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

#include "hb.h" 

@class HBController;
@class PreviewController;
@class PictureFilterController;


//#define HB_NUM_HBLIB_PICTURES      20   // # of preview pictures libhb should generate

@interface PictureController : NSWindowController
{
    hb_handle_t              * fHandle;
    hb_title_t               * fTitle;

    HBController             *fHBController;
    PreviewController        *fPreviewController;        // reference to HBController
    PictureFilterController  *fPictureFilterController;
    
    IBOutlet NSWindow        * fPictureWindow;
    NSMutableDictionary      * fPicturePreviews;        // NSImages, one for each preview libhb creates, created lazily
    int                        fPicture;


    IBOutlet NSBox           * fPictureSizeBox;
    IBOutlet NSBox           * fPictureCropBox;
    IBOutlet NSTextField     * fWidthField;
    IBOutlet NSStepper       * fWidthStepper;
    IBOutlet NSTextField     * fHeightField;
    IBOutlet NSStepper       * fHeightStepper;
    IBOutlet NSButton        * fRatioCheck;
    IBOutlet NSMatrix        * fCropMatrix;
    IBOutlet NSTextField     * fCropTopField;
    IBOutlet NSStepper       * fCropTopStepper;
    IBOutlet NSTextField     * fCropBottomField;
    IBOutlet NSStepper       * fCropBottomStepper;
    IBOutlet NSTextField     * fCropLeftField;
    IBOutlet NSStepper       * fCropLeftStepper;
    IBOutlet NSTextField     * fCropRightField;
    IBOutlet NSStepper       * fCropRightStepper;

	IBOutlet NSPopUpButton   * fAnamorphicPopUp;
    IBOutlet NSTextField     * fInfoField;
	
    IBOutlet NSButton        * fPreviewOpenButton;
    IBOutlet NSButton        * fPictureFiltersOpenButton;
        
    int     MaxOutputWidth;
    int     MaxOutputHeight;
    BOOL    autoCrop;
    BOOL    allowLooseAnamorphic;
    
    int output_width, output_height, output_par_width, output_par_height;
    int display_width;
    
    /* used to track the previous state of the keep aspect
    ratio checkbox when turning anamorphic on, so it can be
    returned to the previous state when anamorphic is turned
    off */
    BOOL    keepAspectRatioPreviousState; 
    
    struct {
        int     detelecine;
        int     deinterlace;
        int     decomb;
        int     denoise;
        int     deblock;
    } fPictureFilterSettings;


}
- (id)init;

- (void) SetHandle: (hb_handle_t *) handle;
- (void) SetTitle:  (hb_title_t *)  title;
- (void)setHBController: (HBController *)controller;
- (IBAction) showPictureWindow: (id)sender;
- (IBAction) showPreviewWindow: (id)sender;
- (IBAction) showFilterWindow: (id)sender;



- (IBAction) SettingsChanged: (id) sender;

- (void)reloadStillPreview;

- (BOOL) autoCrop;
- (void) setAutoCrop: (BOOL) setting;

- (BOOL) allowLooseAnamorphic;
- (void) setAllowLooseAnamorphic: (BOOL) setting;

- (IBAction)showPreviewPanel: (id)sender forTitle: (hb_title_t *)title;


- (void) setToFullScreenMode;
- (void) setToWindowedMode;


@end

