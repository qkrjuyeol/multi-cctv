#pragma once

// 머신에서 Microsoft Visual C++를 사용하여 생성한 IDispatch 래퍼 클래스입니다.

// 참고: 이 파일의 콘텐츠를 수정하지 마세요. Microsoft Visual C++를 통해 이 클래스가 다시 생성될 경우 
// 수정 내용을 덮어씁니다.

/////////////////////////////////////////////////////////////////////////////

#include "afxwin.h"

class CListBox : public CWnd
{
protected:
	DECLARE_DYNCREATE(CListBox)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= {0x8bd21d20, 0xec42, 0x11ce, {0x9e, 0x0d, 0x00, 0xaa, 0x00, 0x60, 0x02, 0xf3}};
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
						const RECT& rect, CWnd* pParentWnd, UINT nID, 
						CCreateContext* pContext = nullptr)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID);
	}

	BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd,
				UINT nID, CFile* pPersist = nullptr, BOOL bStorage = FALSE,
				BSTR bstrLicKey = nullptr)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); 
	}

// 특성
public:
enum fmDropEffect
{
	fmDropEffectNone = 0,
	fmDropEffectCopy = 1,
	fmDropEffectMove = 2,
	fmDropEffectCopyOrMove = 3
};

enum fmAction
{
	fmActionCut = 0,
	fmActionCopy = 1,
	fmActionPaste = 2,
	fmActionDragDrop = 3
};

enum fmMode
{
	fmModeInherit = -2,
	fmModeOn = -1,
	fmModeOff = 0
};

enum fmMousePointer
{
	fmMousePointerDefault = 0,
	fmMousePointerArrow = 1,
	fmMousePointerCross = 2,
	fmMousePointerIBeam = 3,
	fmMousePointerSizeNESW = 6,
	fmMousePointerSizeNS = 7,
	fmMousePointerSizeNWSE = 8,
	fmMousePointerSizeWE = 9,
	fmMousePointerUpArrow = 10,
	fmMousePointerHourGlass = 11,
	fmMousePointerNoDrop = 12,
	fmMousePointerAppStarting = 13,
	fmMousePointerHelp = 14,
	fmMousePointerSizeAll = 15,
	fmMousePointerCustom = 99
};

enum fmScrollBars
{
	fmScrollBarsNone = 0,
	fmScrollBarsHorizontal = 1,
	fmScrollBarsVertical = 2,
	fmScrollBarsBoth = 3
};

enum fmScrollAction
{
	fmScrollActionNoChange = 0,
	fmScrollActionLineUp = 1,
	fmScrollActionLineDown = 2,
	fmScrollActionPageUp = 3,
	fmScrollActionPageDown = 4,
	fmScrollActionBegin = 5,
	fmScrollActionEnd = 6,
	_fmScrollActionAbsoluteChange = 7,
	fmScrollActionPropertyChange = 8,
	fmScrollActionControlRequest = 9,
	fmScrollActionFocusRequest = 10
};

enum fmCycle
{
	fmCycleAllForms = 0,
	fmCycleCurrentForm = 2
};

enum fmZOrder
{
	fmZOrderFront = 0,
	fmZOrderBack = 1
};

enum fmBorderStyle
{
	fmBorderStyleNone = 0,
	fmBorderStyleSingle = 1
};

enum fmTextAlign
{
	fmTextAlignLeft = 1,
	fmTextAlignCenter = 2,
	fmTextAlignRight = 3
};

enum fmAlignment
{
	fmAlignmentLeft = 0,
	fmAlignmentRight = 1
};

enum fmBorders
{
	fmBordersNone = 0,
	fmBordersBox = 1,
	fmBordersLeft = 2,
	fmBordersTop = 3
};

enum fmBackStyle
{
	fmBackStyleTransparent = 0,
	fmBackStyleOpaque = 1
};

enum fmButtonStyle
{
	fmButtonStylePushButton = 0,
	fmButtonStyleToggleButton = 1
};

enum fmPicPosition
{
	fmPicPositionCenter = 0,
	fmPicPositionTopLeft = 1,
	fmPicPositionTopCenter = 2,
	fmPicPositionTopRight = 3,
	fmPicPositionCenterLeft = 4,
	fmPicPositionCenterRight = 5,
	fmPicPositionBottomLeft = 6,
	fmPicPositionBottomCenter = 7,
	fmPicPositionBottomRight = 8
};

enum fmVerticalScrollBarSide
{
	fmVerticalScrollBarSideRight = 0,
	fmVerticalScrollBarSideLeft = 1
};

enum fmLayoutEffect
{
	fmLayoutEffectNone = 0,
	fmLayoutEffectInitiate = 1,
	_fmLayoutEffectRespond = 2
};

enum fmSpecialEffect
{
	fmSpecialEffectFlat = 0,
	fmSpecialEffectRaised = 1,
	fmSpecialEffectSunken = 2,
	fmSpecialEffectEtched = 3,
	fmSpecialEffectBump = 6
};

enum fmDragState
{
	fmDragStateEnter = 0,
	fmDragStateLeave = 1,
	fmDragStateOver = 2
};

enum fmPictureSizeMode
{
	fmPictureSizeModeClip = 0,
	fmPictureSizeModeStretch = 1,
	fmPictureSizeModeZoom = 3
};

enum fmPictureAlignment
{
	fmPictureAlignmentTopLeft = 0,
	fmPictureAlignmentTopRight = 1,
	fmPictureAlignmentCenter = 2,
	fmPictureAlignmentBottomLeft = 3,
	fmPictureAlignmentBottomRight = 4
};

enum fmButtonEffect
{
	fmButtonEffectFlat = 0,
	fmButtonEffectSunken = 2
};

enum fmOrientation
{
	fmOrientationAuto = -1,
	fmOrientationVertical = 0,
	fmOrientationHorizontal = 1
};

enum fmSnapPoint
{
	fmSnapPointTopLeft = 0,
	fmSnapPointTopCenter = 1,
	fmSnapPointTopRight = 2,
	fmSnapPointCenterLeft = 3,
	fmSnapPointCenter = 4,
	fmSnapPointCenterRight = 5,
	fmSnapPointBottomLeft = 6,
	fmSnapPointBottomCenter = 7,
	fmSnapPointBottomRight = 8
};

enum fmPicturePosition
{
	fmPicturePositionLeftTop = 0,
	fmPicturePositionLeftCenter = 1,
	fmPicturePositionLeftBottom = 2,
	fmPicturePositionRightTop = 3,
	fmPicturePositionRightCenter = 4,
	fmPicturePositionRightBottom = 5,
	fmPicturePositionAboveLeft = 6,
	fmPicturePositionAboveCenter = 7,
	fmPicturePositionAboveRight = 8,
	fmPicturePositionBelowLeft = 9,
	fmPicturePositionBelowCenter = 10,
	fmPicturePositionBelowRight = 11,
	fmPicturePositionCenter = 12
};

enum fmDisplayStyle
{
	fmDisplayStyleText = 1,
	fmDisplayStyleList = 2,
	fmDisplayStyleCombo = 3,
	fmDisplayStyleCheckBox = 4,
	fmDisplayStyleOptionButton = 5,
	fmDisplayStyleToggle = 6,
	fmDisplayStyleDropList = 7
};

enum fmShowListWhen
{
	fmShowListWhenNever = 0,
	fmShowListWhenButton = 1,
	fmShowListWhenFocus = 2,
	fmShowListWhenAlways = 3
};

enum fmShowDropButtonWhen
{
	fmShowDropButtonWhenNever = 0,
	fmShowDropButtonWhenFocus = 1,
	fmShowDropButtonWhenAlways = 2
};

enum fmMultiSelect
{
	fmMultiSelectSingle = 0,
	fmMultiSelectMulti = 1,
	fmMultiSelectExtended = 2
};

enum fmListStyle
{
	fmListStylePlain = 0,
	fmListStyleOption = 1
};

enum fmEnterFieldBehavior
{
	fmEnterFieldBehaviorSelectAll = 0,
	fmEnterFieldBehaviorRecallSelection = 1
};

enum fmDragBehavior
{
	fmDragBehaviorDisabled = 0,
	fmDragBehaviorEnabled = 1
};

enum fmMatchEntry
{
	fmMatchEntryFirstLetter = 0,
	fmMatchEntryComplete = 1,
	fmMatchEntryNone = 2
};

enum fmDropButtonStyle
{
	fmDropButtonStylePlain = 0,
	fmDropButtonStyleArrow = 1,
	fmDropButtonStyleEllipsis = 2,
	fmDropButtonStyleReduce = 3
};

enum fmStyle
{
	fmStyleDropDownCombo = 0,
	fmStyleDropDownList = 2
};

enum fmTabOrientation
{
	fmTabOrientationTop = 0,
	fmTabOrientationBottom = 1,
	fmTabOrientationLeft = 2,
	fmTabOrientationRight = 3
};

enum fmTabStyle
{
	fmTabStyleTabs = 0,
	fmTabStyleButtons = 1,
	fmTabStyleNone = 2
};

enum fmIMEMode
{
	fmIMEModeNoControl = 0,
	fmIMEModeOn = 1,
	fmIMEModeOff = 2,
	fmIMEModeDisable = 3,
	fmIMEModeHiragana = 4,
	fmIMEModeKatakana = 5,
	fmIMEModeKatakanaHalf = 6,
	fmIMEModeAlphaFull = 7,
	fmIMEModeAlpha = 8,
	fmIMEModeHangulFull = 9,
	fmIMEModeHangul = 10,
	fmIMEModeHanziFull = 11,
	fmIMEModeHanzi = 12
};

enum fmTransitionEffect
{
	fmTransitionEffectNone = 0,
	fmTransitionEffectCoverUp = 1,
	fmTransitionEffectCoverRightUp = 2,
	fmTransitionEffectCoverRight = 3,
	fmTransitionEffectCoverRightDown = 4,
	fmTransitionEffectCoverDown = 5,
	fmTransitionEffectCoverLeftDown = 6,
	fmTransitionEffectCoverLeft = 7,
	fmTransitionEffectCoverLeftUp = 8,
	fmTransitionEffectPushUp = 9,
	fmTransitionEffectPushRight = 10,
	fmTransitionEffectPushDown = 11,
	fmTransitionEffectPushLeft = 12
};

enum fmListBoxStyles
{
	_fmListBoxStylesNone = 0,
	_fmListBoxStylesListBox = 1,
	_fmListBoxStylesComboBox = 2
};

enum fmRepeatDirection
{
	_fmRepeatDirectionHorizontal = 0,
	_fmRepeatDirectionVertical = 1
};

enum fmEnAutoSize
{
	_fmEnAutoSizeNone = 0,
	_fmEnAutoSizeHorizontal = 1,
	_fmEnAutoSizeVertical = 2,
	_fmEnAutoSizeBoth = 3
};



// 작업
public:
// IMdcList

// 함수
//

	void put_BackColor(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(DISPID_BACKCOLOR, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_BackColor()
	{
		long result;
		InvokeHelper(DISPID_BACKCOLOR, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_BorderColor(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(DISPID_BORDERCOLOR, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_BorderColor()
	{
		long result;
		InvokeHelper(DISPID_BORDERCOLOR, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_BorderStyle(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(DISPID_BORDERSTYLE, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_BorderStyle()
	{
		long result;
		InvokeHelper(DISPID_BORDERSTYLE, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_BordersSuppress(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0x14, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_BordersSuppress()
	{
		BOOL result;
		InvokeHelper(0x14, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_BoundColumn(VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0x1F5, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	VARIANT get_BoundColumn()
	{
		VARIANT result;
		InvokeHelper(0x1F5, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, nullptr);
		return result;
	}

	void put_ColumnCount(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x259, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_ColumnCount()
	{
		long result;
		InvokeHelper(0x259, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_ColumnHeads(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0x25A, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_ColumnHeads()
	{
		BOOL result;
		InvokeHelper(0x25A, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_ColumnWidths(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR;
		InvokeHelper(0x25B, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	CString get_ColumnWidths()
	{
		CString result;
		InvokeHelper(0x25B, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, nullptr);
		return result;
	}

	void put_Enabled(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(DISPID_ENABLED, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_Enabled()
	{
		BOOL result;
		InvokeHelper(DISPID_ENABLED, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put__Font_Reserved(LPDISPATCH newValue)
	{
		static BYTE parms[] = VTS_DISPATCH;
		InvokeHelper(0x7FFFFDFF, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	void putref_Font(LPDISPATCH newValue)
	{
		static BYTE parms[] = VTS_DISPATCH;
		InvokeHelper(DISPID_FONT, DISPATCH_PROPERTYPUTREF, VT_EMPTY, nullptr, parms, newValue);
	}

	LPDISPATCH get_Font()
	{
		LPDISPATCH result;
		InvokeHelper(DISPID_FONT, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, nullptr);
		return result;
	}

	void put_FontBold(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0x3, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_FontBold()
	{
		BOOL result;
		InvokeHelper(0x3, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_FontItalic(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0x4, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_FontItalic()
	{
		BOOL result;
		InvokeHelper(0x4, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_FontName(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR;
		InvokeHelper(0x1, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	CString get_FontName()
	{
		CString result;
		InvokeHelper(0x1, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, nullptr);
		return result;
	}

	void put_FontSize(CY newValue)
	{
		static BYTE parms[] = VTS_CY;
		InvokeHelper(0x2, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, &newValue);
	}

	CY get_FontSize()
	{
		CY result;
		InvokeHelper(0x2, DISPATCH_PROPERTYGET, VT_CY, (void*)&result, nullptr);
		return result;
	}

	void put_FontStrikethru(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0x6, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_FontStrikethru()
	{
		BOOL result;
		InvokeHelper(0x6, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_FontUnderline(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0x5, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_FontUnderline()
	{
		BOOL result;
		InvokeHelper(0x5, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_FontWeight(short newValue)
	{
		static BYTE parms[] = VTS_I2;
		InvokeHelper(0x7, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	short get_FontWeight()
	{
		short result;
		InvokeHelper(0x7, DISPATCH_PROPERTYGET, VT_I2, (void*)&result, nullptr);
		return result;
	}

	void put_ForeColor(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(DISPID_FORECOLOR, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_ForeColor()
	{
		long result;
		InvokeHelper(DISPID_FORECOLOR, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_IntegralHeight(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0x25C, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_IntegralHeight()
	{
		BOOL result;
		InvokeHelper(0x25C, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	long get_ListCount()
	{
		long result;
		InvokeHelper(0xFFFFFDED, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_ListCursor(LPUNKNOWN newValue)
	{
		static BYTE parms[] = VTS_UNKNOWN;
		InvokeHelper(0x193, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	LPUNKNOWN get_ListCursor()
	{
		LPUNKNOWN result;
		InvokeHelper(0x193, DISPATCH_PROPERTYGET, VT_UNKNOWN, (void*)&result, nullptr);
		return result;
	}

	void put_ListIndex(VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0xFFFFFDF2, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	VARIANT get_ListIndex()
	{
		VARIANT result;
		InvokeHelper(0xFFFFFDF2, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, nullptr);
		return result;
	}

	void put_ListStyle(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x133, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_ListStyle()
	{
		long result;
		InvokeHelper(0x133, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_ListWidth(VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0x25E, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	VARIANT get_ListWidth()
	{
		VARIANT result;
		InvokeHelper(0x25E, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, nullptr);
		return result;
	}

	void put_Locked(BOOL newValue)
	{
		static BYTE parms[] = VTS_BOOL;
		InvokeHelper(0xA, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	BOOL get_Locked()
	{
		BOOL result;
		InvokeHelper(0xA, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_MatchEntry(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x1F8, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_MatchEntry()
	{
		long result;
		InvokeHelper(0x1F8, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_MouseIcon(LPDISPATCH newValue)
	{
		static BYTE parms[] = VTS_DISPATCH;
		InvokeHelper(0xFFFFFDF6, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	void putref_MouseIcon(LPDISPATCH newValue)
	{
		static BYTE parms[] = VTS_DISPATCH;
		InvokeHelper(0xFFFFFDF6, DISPATCH_PROPERTYPUTREF, VT_EMPTY, nullptr, parms, newValue);
	}

	LPDISPATCH get_MouseIcon()
	{
		LPDISPATCH result;
		InvokeHelper(0xFFFFFDF6, DISPATCH_PROPERTYGET, VT_DISPATCH, (void*)&result, nullptr);
		return result;
	}

	void put_MousePointer(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0xFFFFFDF7, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_MousePointer()
	{
		long result;
		InvokeHelper(0xFFFFFDF7, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_MultiSelect(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0xFFFFFDEC, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_MultiSelect()
	{
		long result;
		InvokeHelper(0xFFFFFDEC, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_SpecialEffect(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0xC, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_SpecialEffect()
	{
		long result;
		InvokeHelper(0xC, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_Text(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR;
		InvokeHelper(DISPID_TEXT, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	CString get_Text()
	{
		CString result;
		InvokeHelper(DISPID_TEXT, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, nullptr);
		return result;
	}

	void put_TextColumn(VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0x1F6, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	VARIANT get_TextColumn()
	{
		VARIANT result;
		InvokeHelper(0x1F6, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, nullptr);
		return result;
	}

	void put_TopIndex(VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0x263, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	VARIANT get_TopIndex()
	{
		VARIANT result;
		InvokeHelper(0x263, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, nullptr);
		return result;
	}

	BOOL get_Valid()
	{
		BOOL result;
		InvokeHelper(0xFFFFFDF4, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, nullptr);
		return result;
	}

	void put_Value(VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0x0, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	VARIANT get_Value()
	{
		VARIANT result;
		InvokeHelper(0x0, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, nullptr);
		return result;
	}

	VARIANT get_Column(VARIANT * pvargColumn, VARIANT * pvargIndex)
	{
		VARIANT result;
		static BYTE parms[] = VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0xFFFFFDEF, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, parms, pvargColumn, pvargIndex);
		return result;
	}

	void put_Column(VARIANT * pvargColumn, VARIANT * pvargIndex, VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0xFFFFFDEF, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, pvargIndex, newValue);
	}

	VARIANT get_List(VARIANT * pvargIndex, VARIANT * pvargColumn)
	{
		VARIANT result;
		static BYTE parms[] = VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0xFFFFFDF0, DISPATCH_PROPERTYGET, VT_VARIANT, (void*)&result, parms, pvargIndex, pvargColumn);
		return result;
	}

	void put_List(VARIANT * pvargIndex, VARIANT * pvargColumn, VARIANT * newValue)
	{
		static BYTE parms[] = VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0xFFFFFDF0, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, pvargColumn, newValue);
	}

	BOOL get_Selected(VARIANT * pvargIndex)
	{
		BOOL result;
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0xFFFFFDF1, DISPATCH_PROPERTYGET, VT_BOOL, (void*)&result, parms, pvargIndex);
		return result;
	}

	void put_Selected(VARIANT * pvargIndex, BOOL newValue)
	{
		static BYTE parms[] = VTS_PVARIANT VTS_BOOL;
		InvokeHelper(0xFFFFFDF1, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	void AddItem(VARIANT * pvargItem, VARIANT * pvargIndex)
	{
		static BYTE parms[] = VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0xFFFFFDD7, DISPATCH_METHOD, VT_EMPTY, nullptr, parms, pvargItem, pvargIndex);
	}

	void Clear()
	{
		InvokeHelper(0xFFFFFDD6, DISPATCH_METHOD, VT_EMPTY, nullptr, nullptr);
	}

	void RemoveItem(VARIANT * pvargIndex)
	{
		static BYTE parms[] = VTS_PVARIANT;
		InvokeHelper(0xFFFFFDD5, DISPATCH_METHOD, VT_EMPTY, nullptr, parms, pvargIndex);
	}

	void put_IMEMode(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0xFFFFFDE2, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_IMEMode()
	{
		long result;
		InvokeHelper(0xFFFFFDE2, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	long get_DisplayStyle()
	{
		long result;
		InvokeHelper(0xFFFFFDE4, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}

	void put_TextAlign(long newValue)
	{
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x2714, DISPATCH_PROPERTYPUT, VT_EMPTY, nullptr, parms, newValue);
	}

	long get_TextAlign()
	{
		long result;
		InvokeHelper(0x2714, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, nullptr);
		return result;
	}



};
