/**
 * @file llpanellandmarks.cpp
 * @brief Landmarks tab for Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanellandmarks.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "llsdutil.h"

#include "llaccordionctrltab.h"
#include "llagent.h"
#include "llagentui.h"
#include "llfloaterworldmap.h"
#include "llfolderviewitem.h"
#include "llinventorysubtreepanel.h"
#include "lllandmarkactions.h"
#include "llplacesinventorybridge.h"
#include "llsidetray.h"
#include "llviewermenu.h"
#include "llviewerregion.h"

// Not yet implemented; need to remove buildPanel() from constructor when we switch
//static LLRegisterPanelClassWrapper<LLLandmarksPanel> t_landmarks("panel_landmarks");

static const std::string OPTIONS_BUTTON_NAME = "options_gear_btn";
static const std::string ADD_LANDMARK_BUTTON_NAME = "add_landmark_btn";
static const std::string ADD_FOLDER_BUTTON_NAME = "add_folder_btn";
static const std::string TRASH_BUTTON_NAME = "trash_btn";

static const LLPlacesInventoryBridgeBuilder PLACES_INVENTORY_BUILDER;

// helper functions
static void filter_list(LLInventorySubTreePanel* inventory_list, const std::string& string);


LLLandmarksPanel::LLLandmarksPanel()
	:	LLPanelPlacesTab()
	,	mFavoritesInventoryPanel(NULL)
	,	mLandmarksInventoryPanel(NULL)
	,	mMyInventoryPanel(NULL)
	,	mLibraryInventoryPanel(NULL)
	,	mCurrentSelectedList(NULL)
	,	mListCommands(NULL)
	,	mGearFolderMenu(NULL)
	,	mGearLandmarkMenu(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_landmarks.xml");
}

LLLandmarksPanel::~LLLandmarksPanel()
{
}

BOOL LLLandmarksPanel::postBuild()
{
	if (!gInventory.isInventoryUsable())
		return FALSE;

	// mast be called before any other initXXX methods to init Gear menu
	initListCommandsHandlers();

	initFavoritesInventroyPanel();
	initLandmarksInventroyPanel();
	initMyInventroyPanel();
	initLibraryInventroyPanel();

	return TRUE;
}

// virtual
void LLLandmarksPanel::onSearchEdit(const std::string& string)
{
	filter_list(mFavoritesInventoryPanel, string);
	filter_list(mLandmarksInventoryPanel, string);
	filter_list(mMyInventoryPanel, string);
	filter_list(mLibraryInventoryPanel, string);
}

// virtual
void LLLandmarksPanel::onShowOnMap()
{
	if (NULL == mCurrentSelectedList)
	{
		llwarns << "There are no selected list. No actions are performed." << llendl;
		return;
	}
	LLLandmark* landmark = getCurSelectedLandmark();
	if (!landmark)
		return;

	LLVector3d landmark_global_pos;
	if (!landmark->getGlobalPos(landmark_global_pos))
		return;
	
	LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
	if (!landmark_global_pos.isExactlyZero() && worldmap_instance)
	{
		worldmap_instance->trackLocation(landmark_global_pos);
		LLFloaterReg::showInstance("world_map", "center");
	}
}

// virtual
void LLLandmarksPanel::onTeleport()
{
	LLFolderViewItem* current_item = getCurSelectedItem();
	if (!current_item)
	{
		llwarns << "There are no selected list. No actions are performed." << llendl;
		return;
	}

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		listenerp->openItem();
	}
}

// virtual
void LLLandmarksPanel::updateVerbs()
{
	if (!isTabVisible()) 
		return;

	BOOL enabled = isLandmarkSelected();
	mTeleportBtn->setEnabled(enabled);
	mShowOnMapBtn->setEnabled(enabled);

	// TODO: mantipov: Uncomment when mShareBtn is supported
	// Share button should be enabled when neither a folder nor a landmark is selected
	//mShareBtn->setEnabled(NULL != current_item);

	updateListCommands();
}

void LLLandmarksPanel::onSelectionChange(LLInventorySubTreePanel* inventory_list, const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	if (user_action && (items.size() > 0))
	{
		deselectOtherThan(inventory_list);
		mCurrentSelectedList = inventory_list;
	}

	LLFolderViewItem* current_item = inventory_list->getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return;

	updateVerbs();
}

void LLLandmarksPanel::onSelectorButtonClicked()
{
	// TODO: mantipov: update getting of selected item
	// TODO: bind to "i" button
	LLFolderViewItem* cur_item = mFavoritesInventoryPanel->getRootFolder()->getCurSelectedItem();

	LLFolderViewEventListener* listenerp = cur_item->getListener();
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		LLSD key;
		key["type"] = "landmark";
		key["id"] = listenerp->getUUID();

		LLSideTray::getInstance()->showPanel("panel_places", key);
	}
}

//////////////////////////////////////////////////////////////////////////
// PROTECTED METHODS
//////////////////////////////////////////////////////////////////////////

bool LLLandmarksPanel::isLandmarkSelected() const 
{
	LLFolderViewItem* current_item = getCurSelectedItem();
	if(current_item && current_item->getListener()->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		return true;
	}

	return false;
}

LLLandmark* LLLandmarksPanel::getCurSelectedLandmark() const
{

	LLFolderViewItem* cur_item = getCurSelectedItem();
	if(cur_item && cur_item->getListener()->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{ 
		return LLLandmarkActions::getLandmark(cur_item->getListener()->getUUID());
	}
	return NULL;
}

LLFolderViewItem* LLLandmarksPanel::getCurSelectedItem () const 
{
	return mCurrentSelectedList ?  mCurrentSelectedList->getRootFolder()->getCurSelectedItem() : NULL;
}

// virtual
void LLLandmarksPanel::processParcelInfo(const LLParcelData& parcel_data)
{
	//this function will be called after user will try to create a pick for selected landmark.
	// We have to make request to sever to get parcel_id and snaption_id. 
	if(isLandmarkSelected())
	{
		LLLandmark* landmark  =  getCurSelectedLandmark();
		LLFolderViewItem* cur_item = getCurSelectedItem();
		LLUUID id = cur_item->getListener()->getUUID();
		LLInventoryItem* inv_item =  mCurrentSelectedList->getModel()->getItem(id);
		if(landmark)
		{
			LLPanelPick* panel_pick = new LLPanelPick(TRUE);
			LLSD params;
			LLVector3d landmark_global_pos;
			landmark->getGlobalPos(landmark_global_pos);
			panel_pick->prepareNewPick(landmark_global_pos,cur_item->getName(),inv_item->getDescription(),
			parcel_data.snapshot_id,parcel_data.parcel_id);
			// by default save button should be enabled
			panel_pick->childSetEnabled("save_changes_btn", TRUE);
			// let's toggle pick panel into  panel places
			LLPanel* panel_places =  LLSideTray::getInstance()->getChild<LLPanel>("panel_places");//-> sidebar_places
			panel_places->addChild(panel_pick);
			LLRect paren_rect(panel_places->getRect());
			panel_pick->reshape(paren_rect.getWidth(),paren_rect.getHeight(), TRUE);
			panel_pick->setRect(paren_rect);
			params["parcel_id"] =parcel_data.parcel_id;
			/* set exit callback to get back onto panel places  
			 in callback we will make cleaning up( delete pick_panel instance, 
			 remove landmark panel from observer list
			*/ 
			panel_pick->setExitCallback(boost::bind(&LLLandmarksPanel::onPickPanelExit,this,
					panel_pick, panel_places,params));
		}
	}
}

// virtual
void LLLandmarksPanel::setParcelID(const LLUUID& parcel_id)
{
	if (!parcel_id.isNull())
	{
		LLRemoteParcelInfoProcessor::getInstance()->addObserver(parcel_id, this);
		LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(parcel_id);
	}
}

// virtual
void LLLandmarksPanel::setErrorStatus(U32 status, const std::string& reason)
{
	llerrs<< "Can't handle remote parcel request."<< " Http Status: "<< status << ". Reason : "<< reason<<llendl;
}


//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

void LLLandmarksPanel::initFavoritesInventroyPanel()
{
	mFavoritesInventoryPanel = getChild<LLInventorySubTreePanel>("favorites_list");

	LLUUID start_folder_id = mFavoritesInventoryPanel->getModel()->findCategoryUUIDForType(LLAssetType::AT_FAVORITE);

	initLandmarksPanel(mFavoritesInventoryPanel, start_folder_id);

	initAccordion("tab_favorites", mFavoritesInventoryPanel);
}

void LLLandmarksPanel::initLandmarksInventroyPanel()
{
	mLandmarksInventoryPanel = getChild<LLInventorySubTreePanel>("landmarks_list");

	LLUUID start_folder_id = mLandmarksInventoryPanel->getModel()->findCategoryUUIDForType(LLAssetType::AT_LANDMARK);

	initLandmarksPanel(mLandmarksInventoryPanel, start_folder_id);
	mLandmarksInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_ALL_FOLDERS);

	// subscribe to have auto-rename functionality while creating New Folder
	mLandmarksInventoryPanel->setSelectCallback(boost::bind(&LLInventoryPanel::onSelectionChange, mLandmarksInventoryPanel, _1, _2));

	initAccordion("tab_landmarks", mLandmarksInventoryPanel);
}

void LLLandmarksPanel::initMyInventroyPanel()
{
	mMyInventoryPanel= getChild<LLInventorySubTreePanel>("my_inventory_list");

	LLUUID start_folder_id = mMyInventoryPanel->getModel()->getRootFolderID();

	initLandmarksPanel(mMyInventoryPanel, start_folder_id);

	initAccordion("tab_inventory", mMyInventoryPanel);
}

void LLLandmarksPanel::initLibraryInventroyPanel()
{
	mLibraryInventoryPanel = getChild<LLInventorySubTreePanel>("library_list");

	LLUUID start_folder_id = mLibraryInventoryPanel->getModel()->getLibraryRootFolderID();

	initLandmarksPanel(mLibraryInventoryPanel, start_folder_id);

	initAccordion("tab_library", mLibraryInventoryPanel);
}


void LLLandmarksPanel::initLandmarksPanel(LLInventorySubTreePanel* inventory_list, const LLUUID& start_folder_id)
{
	inventory_list->buildSubtreeViewsFor(start_folder_id, &PLACES_INVENTORY_BUILDER);

	inventory_list->setFilterTypes(0x1 << LLInventoryType::IT_LANDMARK);
	inventory_list->setSelectCallback(boost::bind(&LLLandmarksPanel::onSelectionChange, this, inventory_list, _1, _2));

	inventory_list->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);

	LLPlacesFolderView* root_folder = dynamic_cast<LLPlacesFolderView*>(inventory_list->getRootFolder());
	if (root_folder)
	{
		root_folder->setupMenuHandle(LLInventoryType::IT_CATEGORY, mGearFolderMenu->getHandle());
		root_folder->setupMenuHandle(LLInventoryType::IT_LANDMARK, mGearLandmarkMenu->getHandle());
	}
}

void LLLandmarksPanel::initAccordion(const std::string& accordion_tab_name, LLInventorySubTreePanel* inventory_list)
{
	LLAccordionCtrlTab* accordion_tab = getChild<LLAccordionCtrlTab>(accordion_tab_name);
	accordion_tab->setDropDownStateChangedCallback(
		boost::bind(&LLLandmarksPanel::onAccordionExpandedCollapsed, this, _2, inventory_list));
}

void LLLandmarksPanel::onAccordionExpandedCollapsed(const LLSD& param, LLInventorySubTreePanel* inventory_list)
{
	bool expanded = param.asBoolean();

	if(!expanded && (mCurrentSelectedList == inventory_list))
	{
		inventory_list->getRootFolder()->clearSelection();

		mCurrentSelectedList = NULL;
		updateVerbs();
	}
}

void LLLandmarksPanel::deselectOtherThan(const LLInventorySubTreePanel* inventory_list)
{
	if (inventory_list != mFavoritesInventoryPanel)
	{
		mFavoritesInventoryPanel->getRootFolder()->clearSelection();
	}

	if (inventory_list != mLandmarksInventoryPanel)
	{
		mLandmarksInventoryPanel->getRootFolder()->clearSelection();
	}
	if (inventory_list != mMyInventoryPanel)
	{
		mMyInventoryPanel->getRootFolder()->clearSelection();
	}
	if (inventory_list != mLibraryInventoryPanel)
	{
		mLibraryInventoryPanel->getRootFolder()->clearSelection();
	}
}

// List Commands Handlers
void LLLandmarksPanel::initListCommandsHandlers()
{
	mListCommands = getChild<LLPanel>("bottom_panel");

	mListCommands->childSetAction(OPTIONS_BUTTON_NAME, boost::bind(&LLLandmarksPanel::onActionsButtonClick, this));
	mListCommands->childSetAction(ADD_LANDMARK_BUTTON_NAME, boost::bind(&LLLandmarksPanel::onAddLandmarkButtonClick, this));
	mListCommands->childSetAction(ADD_FOLDER_BUTTON_NAME, boost::bind(&LLLandmarksPanel::onAddFolderButtonClick, this));
	mListCommands->childSetAction(TRASH_BUTTON_NAME, boost::bind(&LLLandmarksPanel::onTrashButtonClick, this));

	
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Add.Action", boost::bind(&LLLandmarksPanel::onAddAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.CopyPaste.Action", boost::bind(&LLLandmarksPanel::onCopyPasteAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Custom.Action", boost::bind(&LLLandmarksPanel::onCustomAction, this, _2));
	mCommitCallbackRegistrar.add("Places.LandmarksGear.Folding.Action", boost::bind(&LLLandmarksPanel::onFoldingAction, this, _2));
	mEnableCallbackRegistrar.add("Places.LandmarksGear.Enable", boost::bind(&LLLandmarksPanel::isActionEnabled, this, _2));
	mGearLandmarkMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_places_gear_landmark.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	mGearFolderMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_places_gear_folder.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}


void LLLandmarksPanel::updateListCommands()
{
	// TODO: should be false when "Received" folder is selected
	bool add_folder_enabled = mCurrentSelectedList == mLandmarksInventoryPanel;
	bool trash_enabled = false; // TODO: should be false when "Received" folder is selected

	LLFolderViewItem* current_item = getCurSelectedItem();

	if (current_item)
	{
		LLFolderViewEventListener* listenerp = current_item->getListener();
		if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
		{
			trash_enabled = mCurrentSelectedList != mLibraryInventoryPanel;
		}
	}

	// keep Options & Add Landmark buttons always enabled
	mListCommands->childSetEnabled(ADD_FOLDER_BUTTON_NAME, add_folder_enabled);
	mListCommands->childSetEnabled(TRASH_BUTTON_NAME, trash_enabled);
}

void LLLandmarksPanel::onActionsButtonClick()
{
	LLFolderViewItem* cur_item = NULL;
	if(mCurrentSelectedList)
		cur_item = mCurrentSelectedList->getRootFolder()->getCurSelectedItem();
	
	if(!cur_item)
		return;
	
	LLFolderViewEventListener* listenerp = cur_item->getListener();
	
	LLMenuGL* menu  =NULL;
	if (listenerp->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{
		menu = mGearLandmarkMenu;
	}
	else if (listenerp->getInventoryType() == LLInventoryType::IT_CATEGORY)
	{
		mGearFolderMenu->getChild<LLMenuItemCallGL>("expand")->setVisible(!cur_item->isOpen());
		mGearFolderMenu->getChild<LLMenuItemCallGL>("collapse")->setVisible(cur_item->isOpen());
		menu = mGearFolderMenu;
	}
	if(menu)
	{
		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLView* actions_btn  = getChild<LLView>(OPTIONS_BUTTON_NAME);
		S32 menu_x, menu_y;
		actions_btn->localPointToOtherView(0,actions_btn->getRect().getHeight(),&menu_x,&menu_y, this);
		menu_y += menu->getRect().getHeight();
		LLMenuGL::showPopup(this, menu, menu_x,menu_y);
	}
}

void LLLandmarksPanel::onAddLandmarkButtonClick() const
{
	if(LLLandmarkActions::landmarkAlreadyExists())
	{
		std::string location;
		LLAgentUI::buildLocationString(location, LLAgentUI::LOCATION_FORMAT_FULL);
		llwarns<<" Landmark already exists at location:  "<< location<<llendl;
		return;
	}
	LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "create_landmark"));
}

void LLLandmarksPanel::onAddFolderButtonClick() const
{
	LLFolderViewItem*  item = getCurSelectedItem();
	if(item &&  mCurrentSelectedList == mLandmarksInventoryPanel)
	{
		LLFolderBridge *parentBridge = NULL;
		if(item-> getListener()->getInventoryType() == LLInventoryType::IT_LANDMARK)
		{
			parentBridge = dynamic_cast<LLFolderBridge*>(item->getParentFolder()->getListener());
			/*WORKAROUND:* 
			 LLFolderView::doIdle() is calling in each frame,
			 it changes selected items before LLFolderView::startRenamingSelectedItem.
			 To avoid it we have to change keyboardFocus. 
			*/
			gFocusMgr.setKeyboardFocus(item->getParentFolder());
		}
		else if (item-> getListener()->getInventoryType() == LLInventoryType::IT_CATEGORY) 
		{
			parentBridge = dynamic_cast<LLFolderBridge*>(item->getListener());
			gFocusMgr.setKeyboardFocus(item);
		}
		menu_create_inventory_item(mCurrentSelectedList->getRootFolder(),parentBridge, LLSD("category"));
	}
}

void LLLandmarksPanel::onTrashButtonClick() const
{
	if(!mCurrentSelectedList) return;

	mCurrentSelectedList->getRootFolder()->doToSelected(mCurrentSelectedList->getModel(), "delete");
}

void LLLandmarksPanel::onAddAction(const LLSD& userdata) const
{
	std::string command_name = userdata.asString();
	if("add_landmark" == command_name)
	{
		onAddLandmarkButtonClick();
	} 
	else if ("category" == command_name)
	{
		onAddFolderButtonClick();
	}
}

void LLLandmarksPanel::onCopyPasteAction(const LLSD& userdata) const
{
	if(!mCurrentSelectedList) 
		return;
	std::string command_name = userdata.asString();
    if("copy_slurl" == command_name)
	{
    	LLFolderViewItem* cur_item = getCurSelectedItem();
		if(cur_item)
			LLLandmarkActions::copySLURLtoClipboard(cur_item->getListener()->getUUID());
	}
	else if ( "paste" == command_name)
	{
		mCurrentSelectedList->getRootFolder()->paste();
	} 
	else if ( "cut" == command_name)
	{
		mCurrentSelectedList->getRootFolder()->cut();
	}
	else
	{
		mCurrentSelectedList->getRootFolder()->doToSelected(mCurrentSelectedList->getModel(),command_name);
	}
}

void LLLandmarksPanel::onFoldingAction(const LLSD& userdata) const
{
	if(!mCurrentSelectedList) return;

	LLFolderView* root_folder = mCurrentSelectedList->getRootFolder();
	std::string command_name = userdata.asString();

	if ("expand_all" == command_name)
	{
		root_folder->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_DOWN);
		root_folder->arrangeAll();
	}
	else if ("collapse_all" == command_name)
	{
		root_folder->closeAllFolders();
	}
	else
	{
		root_folder->doToSelected(&gInventory, userdata);
	}
}

bool LLLandmarksPanel::isActionEnabled(const LLSD& userdata) const
{
	std::string command_name = userdata.asString();
	if("category" == command_name)
	{
		return mCurrentSelectedList == mLandmarksInventoryPanel; 
	}
	else if("paste" == command_name)
	{
		return mCurrentSelectedList ? mCurrentSelectedList->getRootFolder()->canPaste() : false;
	}
	return true;
}

void LLLandmarksPanel::onCustomAction(const LLSD& userdata)
{
	LLFolderViewItem* cur_item = getCurSelectedItem();
	if(!cur_item)
		return ;
	std::string command_name = userdata.asString();
	if("more_info" == command_name)
	{
		cur_item->getListener()->performAction(mCurrentSelectedList->getRootFolder(),mCurrentSelectedList->getModel(),"about");
	}
	else if ("teleport" == command_name)
	{
		onTeleport();
	}
	else if ("show_on_map" == command_name)
	{
		onShowOnMap();
	}
	else if ("create_pick" == command_name)
	{
		LLLandmark* landmark = getCurSelectedLandmark();
		if(!landmark) return;
		
		LLViewerRegion* region = gAgent.getRegion();
		if (!region) return;

		LLGlobalVec pos_global;
		LLUUID region_id;
		landmark->getGlobalPos(pos_global);
		landmark->getRegionID(region_id);
		LLVector3 region_pos((F32)fmod(pos_global.mdV[VX], (F64)REGION_WIDTH_METERS),
						  (F32)fmod(pos_global.mdV[VY], (F64)REGION_WIDTH_METERS),
						  (F32)pos_global.mdV[VZ]);

		LLSD body;
		std::string url = region->getCapability("RemoteParcelRequest");
		if (!url.empty())
		{
			body["location"] = ll_sd_from_vector3(region_pos);
			if (!region_id.isNull())
			{
				body["region_id"] = region_id;
			}
			if (!pos_global.isExactlyZero())
			{
				U64 region_handle = to_region_handle(pos_global);
				body["region_handle"] = ll_sd_from_U64(region_handle);
			}
			LLHTTPClient::post(url, body, new LLRemoteParcelRequestResponder(getObserverHandle()));
		}
		else 
		{
			llwarns << "Can't create pick for landmark for region" << region_id 
					<< ". Region: "	<< region->getName() 
					<< " does not support RemoteParcelRequest" << llendl; 
		}
	}
}

void LLLandmarksPanel::onPickPanelExit( LLPanelPick* pick_panel, LLView* owner, const LLSD& params)
{
	pick_panel->setVisible(FALSE);
	owner->removeChild(pick_panel);
	//we need remove  observer to  avoid  processParcelInfo in the future.
	LLRemoteParcelInfoProcessor::getInstance()->removeObserver(params["parcel_id"].asUUID(), this);

	delete pick_panel;
	pick_panel = NULL;
}


//////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//////////////////////////////////////////////////////////////////////////
static void filter_list(LLInventorySubTreePanel* inventory_list, const std::string& string)
{
	if (string == "")
	{
		inventory_list->setFilterSubString(LLStringUtil::null);

		// re-open folders that were initially open
		inventory_list->restoreFolderState();
	}

	gInventory.startBackgroundFetch();

	if (inventory_list->getFilterSubString().empty() && string.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}

	// save current folder open state if no filter currently applied
	if (inventory_list->getRootFolder()->getFilterSubString().empty())
	{
		inventory_list->saveFolderState();
	}

	// set new filter string
	inventory_list->setFilterSubString(string);
}
// EOF
