
#include "std.h"
#include "gxgraphics.h"
#include "gxruntime.h"
#include <Windows.h>
#include <afx.h>
#include <bbruntime/bbsys.h>
#include "compiler/ex.h"

extern gxRuntime *gx_runtime;

gxGraphics::gxGraphics( gxRuntime *rt,IDirectDraw7 *dd,IDirectDrawSurface7 *fs,IDirectDrawSurface7 *bs,bool d3d ):
runtime(rt),dirDraw(dd),dir3d(0),dir3dDev(0),def_font(0),gfx_lost(false),dummy_mesh(0){

	dirDraw->QueryInterface( IID_IDirectDraw,(void**)&ds_dirDraw );

	front_canvas=d_new gxCanvas( this,fs,0 );
	back_canvas=d_new gxCanvas( this,bs,0 );

	front_canvas->cls();
	back_canvas->cls();

	def_font=loadFont( "Courier",12,0 );

	front_canvas->setFont( def_font );
	back_canvas->setFont( def_font );

	memset(&primFmt,0,sizeof(primFmt));
	primFmt.dwSize=sizeof(primFmt);
	fs->GetPixelFormat( &primFmt );

	//are we fullscreen?
	_gamma=0;
	if( fs!=bs ){
		if( fs->QueryInterface( IID_IDirectDrawGammaControl,(void**)&_gamma )>=0 ){
			if( _gamma->GetGammaRamp( 0,&_gammaRamp )<0 ) _gamma=0;
		}
	}
	if( !_gamma ){
		for( int k=0;k<256;++k ) _gammaRamp.red[k]=_gammaRamp.blue[k]=_gammaRamp.green[k]=k;
	}
}

gxGraphics::~gxGraphics(){
	if( _gamma ) _gamma->Release();
	while( scene_set.size() ) freeScene( *scene_set.begin() );
	while( movie_set.size() ) closeMovie( *movie_set.begin() );
	while( font_set.size() ) freeFont( *font_set.begin() );
	while( canvas_set.size() ) freeCanvas( *canvas_set.begin() );

	set<string>::iterator it;
	for( it=font_res.begin();it!=font_res.end();++it ) RemoveFontResource( (*it).c_str() );
	font_res.clear();

	delete back_canvas;
	delete front_canvas;

	ds_dirDraw->Release();

	dirDraw->RestoreDisplayMode();
	dirDraw->Release();
}

void gxGraphics::setGamma( int r,int g,int b,float dr,float dg,float db ){
	_gammaRamp.red[r&255]=dr*257.0f;
	_gammaRamp.green[g&255]=dg*257.0f;
	_gammaRamp.blue[b&255]=db*257.0f;
}

void gxGraphics::updateGamma( bool calibrate ){
	if( !_gamma ) return;
	_gamma->SetGammaRamp( calibrate ? DDSGR_CALIBRATE : 0,&_gammaRamp );
}

void gxGraphics::getGamma( int r,int g,int b,float *dr,float *dg,float *db ){
	*dr=_gammaRamp.red[r&255]/257.0f;
	*dg=_gammaRamp.green[g&255]/257.0f;
	*db=_gammaRamp.blue[b&255]/257.0f;
}

void gxGraphics::backup(){
}

bool gxGraphics::restore(){

	while( dirDraw->TestCooperativeLevel()!=DD_OK ){

		if( dirDraw->TestCooperativeLevel()==DDERR_WRONGMODE ) return false;

		Sleep( 100 );
	}

	if( back_canvas->getSurface()->IsLost()==DD_OK ) return true;

	dirDraw->RestoreAllSurfaces();

	//restore all canvases
	set<gxCanvas*>::iterator it;
	for( it=canvas_set.begin();it!=canvas_set.end();++it ){
		(*it)->restore();
	}

	//restore all meshes (b3d surfaces)
	set<gxMesh*>::iterator mesh_it;
	for( mesh_it=mesh_set.begin();mesh_it!=mesh_set.end();++mesh_it ){
		(*mesh_it)->restore();
	}
	if( dir3d ) dir3d->EvictManagedTextures();

	return true;
}

gxCanvas *gxGraphics::getFrontCanvas()const{
	return front_canvas;
}

gxCanvas *gxGraphics::getBackCanvas()const{
	return back_canvas;
}

gxFont *gxGraphics::getDefaultFont()const{
	return def_font;
}

void gxGraphics::vwait(){
	dirDraw->WaitForVerticalBlank( DDWAITVB_BLOCKBEGIN,0 );
}

void gxGraphics::flip( bool v ){
	runtime->flip( v );
}

void gxGraphics::copy( gxCanvas *dest,int dx,int dy,int dw,int dh,gxCanvas *src,int sx,int sy,int sw,int sh ){
	RECT r={ dx,dy,dx+dw,dy+dh };
	ddUtil::copy( dest->getSurface(),dx,dy,dw,dh,src->getSurface(),sx,sy,sw,sh );
	dest->damage( r );
}

int gxGraphics::getScanLine()const{
	DWORD t=0;
	dirDraw->GetScanLine( &t );
	return t;
}

int gxGraphics::getTotalVidmem()const{
	DDCAPS caps={sizeof(caps)};
	dirDraw->GetCaps( &caps,0 );
	return caps.dwVidMemTotal;
}

int gxGraphics::getAvailVidmem()const{
	DDCAPS caps={sizeof(caps)};
	dirDraw->GetCaps( &caps,0 );
	return caps.dwVidMemFree;
}

gxMovie *gxGraphics::openMovie( const string &file,int flags ){

	IAMMultiMediaStream *iam_stream;

	if( CoCreateInstance(
		CLSID_AMMultiMediaStream,NULL,CLSCTX_INPROC_SERVER,
        IID_IAMMultiMediaStream,(void **)&iam_stream )==S_OK ){

		if( iam_stream->Initialize( STREAMTYPE_READ,AMMSF_NOGRAPHTHREAD,NULL )==S_OK ){

			if( iam_stream->AddMediaStream( ds_dirDraw,&MSPID_PrimaryVideo,0,NULL )==S_OK ){

				iam_stream->AddMediaStream( NULL,&MSPID_PrimaryAudio,AMMSF_ADDDEFAULTRENDERER,NULL );

				WCHAR *path=new WCHAR[ file.size()+1 ];
				MultiByteToWideChar( CP_ACP,0,file.c_str(),-1,path,sizeof(WCHAR)*(file.size()+1) );
				int n=iam_stream->OpenFile( path,0 );
				delete path;

				if( n==S_OK ){
					gxMovie *movie=d_new gxMovie( this,iam_stream );
					movie_set.insert( movie );
					return movie;
				}
			}
		}
		iam_stream->Release();
	}
	return 0;
}

gxMovie *gxGraphics::verifyMovie( gxMovie *m ){
	return movie_set.count( m ) ? m : 0;
}

void gxGraphics::closeMovie( gxMovie *m ){
	if( movie_set.erase( m ) ) delete m;
}

gxCanvas *gxGraphics::createCanvas( int w,int h,int flags ){
	ddSurf *s=ddUtil::createSurface( w,h,flags,this );
	if( !s ) return 0;
	gxCanvas *c=d_new gxCanvas( this,s,flags );
	canvas_set.insert( c );
	c->cls();
	return c;
}

gxCanvas *gxGraphics::loadCanvas( const string &f,int flags ){
	ddSurf *s=ddUtil::loadSurface( f,flags,this );
	if( !s ) return 0;
	gxCanvas *c=d_new gxCanvas( this,s,flags );
	canvas_set.insert( c );
	return c;
}

gxCanvas *gxGraphics::verifyCanvas( gxCanvas *c ){
	return canvas_set.count( c ) || c==front_canvas || c==back_canvas ? c : 0;
}

void gxGraphics::freeCanvas( gxCanvas *c ){
	if( canvas_set.erase( c ) ) delete c;
}

int gxGraphics::getWidth()const{
	return front_canvas->getWidth();
}

int gxGraphics::getHeight()const{
	return front_canvas->getHeight();
}

int gxGraphics::getDepth()const{
	return front_canvas->getDepth();
}

// god forgive me, gonna MANUALLY parse a ttf

// -------------------------------------------------------------------------------------
// based on https://www.codeproject.com/Articles/2293/Retrieving-Font-Name-from-TTF-File
// -------------------------------------------------------------------------------------

//This is TTF file header
typedef struct _tagTT_OFFSET_TABLE {
	USHORT uMajorVersion;
	USHORT uMinorVersion;
	USHORT uNumOfTables;
	USHORT uSearchRange;
	USHORT uEntrySelector;
	USHORT uRangeShift;
}TT_OFFSET_TABLE;

//Tables in TTF file and there placement and name (tag)
typedef struct _tagTT_TABLE_DIRECTORY {
	char szTag[4]; //table name
	ULONG uCheckSum; //Check sum
	ULONG uOffset; //Offset from beginning of file
	ULONG uLength; //length of the table in bytes
}TT_TABLE_DIRECTORY;

//Header of names table
typedef struct _tagTT_NAME_TABLE_HEADER {
	USHORT uFSelector; //format selector. Always 0
	USHORT uNRCount; //Name Records count
	USHORT uStorageOffset; //Offset for strings storage, 
	//from start of the table
}TT_NAME_TABLE_HEADER;

//Record in names table
typedef struct _tagTT_NAME_RECORD {
	USHORT uPlatformID;
	USHORT uEncodingID;
	USHORT uLanguageID;
	USHORT uNameID;
	USHORT uStringLength;
	USHORT uStringOffset; //from start of storage area
}TT_NAME_RECORD;

typedef struct _tagTT_HEAD_RECORD {
	float     fVersion;
	float     fFontRevision;
	uint32_t  uCheckSumAdjustment;
	uint32_t  uMagicNumber; //always 0x5F0F3CF5
	uint16_t  uFlags;
	uint16_t  uUnitsPerEm;
	long long lCreated;
	long long lNodified;
	short     sXMin;
	short     sYMin;
	short     sXMax;
	short     sYMax;
	uint16_t  macStyle;
	uint16_t  lowestRecPPEM;
	short     fontDirectionHintl;
	short     indexToLocFormat;
	short     glyphDataFormat; //from start of storage area
}TT_HEAD_RECORD;

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

string gxGraphics::getTTFInternalName(string lpszFilePath)
{
	CFile f;
	CString csRetVal;

	// error returns
	char* retTM = "FAILURE - CORRUPT TTF: ";
	char* retTM2 = "FAILURE - INVALID TTF: ";
	char* retTM3 = "FAILURE - CORRUPT TTF: ";
	char* retTM4 = "FAILURE - UNABLE TO READ TTF: ";

	try
	{
		//lpszFilePath is the path to our font file
		if (f.Open(lpszFilePath.c_str(), CFile::modeRead | CFile::shareDenyWrite)) {

			//define and read file header
			TT_OFFSET_TABLE ttOffsetTable;
			f.Read(&ttOffsetTable, sizeof(TT_OFFSET_TABLE));

			//remember to rearrange bytes in the field you gonna use
			ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
			ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
			ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);

			//check is this is a true type font and the version is 1.0
			if (ttOffsetTable.uMajorVersion != 1)
				return retTM + (CString)lpszFilePath.c_str() + " | DID YOU CHANGE THE FILE EXTENSION?";

			//ensure the version is 1.0
			if (ttOffsetTable.uMinorVersion != 0)
				return retTM2 + (CString)lpszFilePath.c_str() + " | FILE VERSION IS 1." + (char)ttOffsetTable.uMinorVersion + ", EXPECTED: 1.0";

			TT_TABLE_DIRECTORY tblDir;
			BOOL bFoundName = FALSE;
			CString csTemp;

			for (int i = 0; i < ttOffsetTable.uNumOfTables; i++) {
				f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
				csTemp.Empty();

				//table's tag cannot exceed 4 characters
				strncpy(csTemp.GetBuffer(4), tblDir.szTag, 4);
				csTemp.ReleaseBuffer();
				if (csTemp.CompareNoCase(_T("name")) == 0) {
					//we found our table. Rearrange order and quit the loop
					bFoundName = TRUE;
					tblDir.uLength = SWAPLONG(tblDir.uLength);
					tblDir.uOffset = SWAPLONG(tblDir.uOffset);
					break;
				}
			}

			if (bFoundName) {
				//move to offset we got from Offsets Table
				f.Seek(tblDir.uOffset, CFile::begin);
				TT_NAME_TABLE_HEADER ttNTHeader;
				f.Read(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER));

				//again, don't forget to swap bytes!
				ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
				ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
				TT_NAME_RECORD ttRecord;
				bFoundName = FALSE;

				for (int i = 0; i < ttNTHeader.uNRCount; i++) {
					f.Read(&ttRecord, sizeof(TT_NAME_RECORD));
					ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);

					//1 says that this is font name. 0 for example determines copyright info
					if (ttRecord.uNameID == 1) {
						ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
						ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);

						//save file position, so we can return to continue with search
						int nPos = f.GetPosition();
						f.Seek(tblDir.uOffset + ttRecord.uStringOffset +
							ttNTHeader.uStorageOffset, CFile::begin);

						//bug fix: see the post by SimonSays to read more about it
						//std::string* lpszNameBuf = (std::string*)malloc(ttRecord.uStringLength+1);
						//ZeroMemory(&lpszNameBuf, ttRecord.uStringLength+1);
						//f.Read(&lpszNameBuf, ttRecord.uStringLength);

						csTemp.Empty();
						csTemp.FreeExtra();

						f.Read(csTemp.GetBuffer(ttRecord.uStringLength), ttRecord.uStringLength+1);

						csTemp.ReleaseBuffer();

						//CString retTmp = CString(lpszNameBuf);
						//return lpszNameBuf;

						//yes, still need to check if the font name is not empty
						//if it is, continue the search
						if (!(csTemp.IsEmpty())) {
							csRetVal = csTemp;
							//return csTemp;
							break;
						}
						//csTemp.ReleaseBuffer();
						f.Seek(nPos, CFile::begin);
					}
				}

				/*BOOL bFoundHead = FALSE;
				
				for (int j = 0; j < ttOffsetTable.uNumOfTables; j++) {
					f.Read(&tblDir, sizeof(TT_TABLE_DIRECTORY));
					csTemp.Empty();

					//table's tag cannot exceed 4 characters
					strncpy(csTemp.GetBuffer(4), tblDir.szTag, 4);
					csTemp.ReleaseBuffer();
					if (csTemp.CompareNoCase(_T("head")) == 0) {
						//we found our table. Rearrange order and quit the loop
						bFoundHead = TRUE;
						//tblDir.uLength = SWAPLONG(tblDir.uLength);
						//tblDir.uOffset = SWAPLONG(tblDir.uOffset);
						break;
					}
				}

				if (bFoundHead) {
					//move to offset we got from Offsets Table
					f.Seek(tblDir.uOffset, CFile::begin);
					
					TT_HEAD_RECORD ttHRecord;
					bFoundHead = FALSE;

					f.Read(&ttHRecord, sizeof(TT_HEAD_RECORD));
					ttHRecord.macStyle = SWAPWORD(ttHRecord.macStyle);

					int tmp = ttHRecord.macStyle & 0x0001 == 0x0001;
					gx_runtime->debugLog((const char*)tmp);

					if (tmp) {
						csRetVal += " Bold";
					}
				}
				else
				{
					f.Close();
					//return retTM3 + (CString)lpszFilePath.c_str() + " | HEAD FIELD DOES NOT EXIST";
					gx_runtime->debugLog("LOADFONT - WARNING: " + (CString)lpszFilePath.c_str() + " DOES NOT CONTAIN FIELD: HEAD");
				}*/
			}
			else
			{
				f.Close();
				return retTM3 + (CString)lpszFilePath.c_str() + " | NAME FIELD DOES NOT EXIST";
			}
			f.Close();
			return csRetVal;
		}

		return retTM4 + (CString)lpszFilePath.c_str() + " | CONTACT FUNNIMAN.EXE";
	}
	catch (Ex exc)
	{
		RTEX(exc.ex.c_str());
	}
}

gxFont *gxGraphics::loadFont( const string &f,int height,int flags ){

	int bold=flags & gxFont::FONT_BOLD ? FW_BOLD : FW_REGULAR;
	int italic=flags & gxFont::FONT_ITALIC ? 1 : 0;
	int underline=flags & gxFont::FONT_UNDERLINE ? 1 : 0;
	int strikeout=0;

	string t;
	int n=f.find('.');
	if( n!=string::npos ){
		t=fullfilename(f);
		//gx_runtime->debugLog(t.c_str()); // temp, to determine out how fucked loadfont is
		if( !font_res.count(t) && AddFontResource( t.c_str() ) ) font_res.insert( t );

		// getTTFInternalName can only get a true type font's internal name, if not ttf: use old font loader
		if (t.find(".ttf") != std::string::npos)
		{
			// new font loader
			string rInput;

			rInput = getTTFInternalName(t);

			//if (tmp.Find("FAILURE -") != -1)
			if (rInput.find("FAILURE -") != std::string::npos)
			{
				if (debug) {
					RTEX(rInput.c_str());
				} else {
					errorLog.push_back("LoadFont: "+rInput);
				}
				return 0;
			}

			//gx_runtime->debugLog(rInput.c_str()); // temp, to determine out how fucked loadfont is
			t = rInput;
			//gx_runtime->debugLog(t.c_str()); // temp, to determine out how fucked loadfont is
		} else {
			// old font loader
			t = filenamefile(f.substr(0, n)); // causes font loading bug
			//gx_runtime->debugLog(t.c_str()); // temp, to determine out how fucked loadfont is
		}
	}else{
		t=f;
	}

	//save and turn off font smoothing....
	BOOL smoothing=FALSE;
	SystemParametersInfo( SPI_GETFONTSMOOTHING,0,&smoothing,0 );
	SystemParametersInfo( SPI_SETFONTSMOOTHING,FALSE,0,0 );

	HFONT hfont=CreateFont( 
		height,0,0,0,
		bold,italic,underline,strikeout,
		ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
		DEFAULT_PITCH|FF_DONTCARE,t.c_str() );

	if( !hfont ){
		//restore font smoothing
		SystemParametersInfo( SPI_SETFONTSMOOTHING,smoothing,0,0 );
		return 0;
	}

	HDC hdc=CreateCompatibleDC( 0 );
	HFONT pfont=(HFONT)SelectObject( hdc,hfont );

	TEXTMETRIC tm={0};
	if( !GetTextMetrics( hdc,&tm ) ){
		SelectObject( hdc,pfont );
		DeleteDC( hdc );
		DeleteObject( hfont );
		SystemParametersInfo( SPI_SETFONTSMOOTHING,smoothing,0,0 );
		return 0;
	}
	height=tm.tmHeight;

	int first=tm.tmFirstChar,last=tm.tmLastChar;
	int sz=last-first+1;
	int *offs=d_new int[sz];
	int *widths=d_new int[sz];
	int *as=d_new int[sz];

	//calc size of canvas to hold font.
	int x=0,y=0,max_x=0;
	for( int k=0;k<sz;++k ){

		char t=k+first;

		SIZE sz;
		GetTextExtentPoint32( hdc,&t,1,&sz );
		int w=sz.cx;

		as[k]=0;

		ABC abc;
		if( GetCharABCWidths( hdc,t,t,&abc ) ){
			if( abc.abcA<0 ){
				as[k]=ceil(-abc.abcA);
				w+=as[k];
			}
			if( abc.abcC<0 ) w+=ceil(-abc.abcC);
		}

		if( x && x+w>getWidth() ){ x=0;y+=height; }
		offs[k]=(x<<16)|y;
		widths[k]=w;
		x+=w;if( x>max_x ) max_x=x;
	}
	SelectObject( hdc,pfont );
	DeleteDC( hdc );

	int cw=max_x,ch=y+height;

	if( gxCanvas *c=createCanvas( cw,ch,0 ) ){
		ddSurf *surf=c->getSurface();

		HDC surf_hdc;
		if( surf->GetDC( &surf_hdc )>=0 ){
			HFONT pfont=(HFONT)SelectObject( surf_hdc,hfont );

			SetBkColor( surf_hdc,0x000000 );
			SetTextColor( surf_hdc,0xffffff );

			for( int k=0;k<sz;++k ){
				int x=(offs[k]>>16)&0xffff,y=offs[k]&0xffff;
				char t=k+first;
				RECT rect={x,y,x+widths[k],y+height};
				ExtTextOut( surf_hdc,x+as[k],y,ETO_CLIPPED,&rect,&t,1,0 );
			}

			SelectObject( surf_hdc,pfont );
			surf->ReleaseDC( surf_hdc );
			DeleteObject( hfont );
			delete[] as;

			c->backup();
			gxFont *font=d_new gxFont( this,c,tm.tmMaxCharWidth,height,first,last+1,tm.tmDefaultChar,offs,widths );
			font_set.insert( font );

			//restore font smoothing
			SystemParametersInfo( SPI_SETFONTSMOOTHING,smoothing,0,0 );
			return font;
		}else{
		}
		freeCanvas( c );
	}else{
	}
	DeleteObject( hfont );
	delete[] as;
	delete[] widths;
	delete[] offs;

	//restore font smoothing
	SystemParametersInfo( SPI_SETFONTSMOOTHING,smoothing,0,0 );
	return 0;
}

gxFont *gxGraphics::verifyFont( gxFont *f ){
	return font_set.count( f ) ? f : 0;
}

void gxGraphics::freeFont( gxFont *f ){
	if( font_set.erase( f ) ) delete f;
}

//////////////
// 3D STUFF //
//////////////

static int maxDevType;

static HRESULT CALLBACK enumDevice( char *desc,char *name,D3DDEVICEDESC7 *devDesc,void *context ){
	gxGraphics *g=(gxGraphics*)context;
	int t=0;
	GUID guid=devDesc->deviceGUID;
	if( guid==IID_IDirect3DRGBDevice ) t=1;
	else if( guid==IID_IDirect3DHALDevice ) t=2;
	else if( guid==IID_IDirect3DTnLHalDevice ) t=3;
	if( t>maxDevType ){
		g->dir3dDevDesc=*devDesc;
		maxDevType=t;
	}
	return D3DENUMRET_OK;
}

static HRESULT CALLBACK enumZbuffFormat( LPDDPIXELFORMAT format,void *context ){
	gxGraphics *g=(gxGraphics*)context;
	if( format->dwZBufferBitDepth==g->primFmt.dwRGBBitCount ){
		g->zbuffFmt=*format;
		return D3DENUMRET_CANCEL;
	}
	if( format->dwZBufferBitDepth>g->zbuffFmt.dwZBufferBitDepth ){
		if( format->dwZBufferBitDepth<g->primFmt.dwRGBBitCount ){
			g->zbuffFmt=*format;
		}
	}
	return D3DENUMRET_OK;
}

struct TexFmt{
	DDPIXELFORMAT fmt;
	int bits,a_bits,rgb_bits;
};

static int cntBits( int mask ){
	int n=0;
	for( int k=0;k<32;++k ){
		if( mask & (1<<k) ) ++n;
	}
	return n;
}

static vector<TexFmt> tex_fmts;

static HRESULT CALLBACK enumTextureFormat( DDPIXELFORMAT *fmt,void *p ){
	TexFmt t;
	t.fmt=*fmt;
	t.bits=fmt->dwRGBBitCount;
	t.a_bits=(fmt->dwFlags & DDPF_ALPHAPIXELS) ? cntBits(fmt->dwRGBAlphaBitMask) : 0;
	t.rgb_bits=(fmt->dwFlags & DDPF_RGB) ? cntBits(fmt->dwRBitMask|fmt->dwGBitMask|fmt->dwBBitMask) : 0;

	tex_fmts.push_back( t );

	return D3DENUMRET_OK;
}

static string itobin( int n ){
	string t;
	for( int k=0;k<32;n<<=1,++k ){
		t+=(n&0x80000000) ? '1' : '0';
	}
	return t;
}

static void debugPF( const DDPIXELFORMAT &pf ){
	string t;
	t="Bits:"+itoa( pf.dwRGBBitCount );
	gx_runtime->debugLog( t.c_str() );
	t="R Mask:"+itobin( pf.dwRBitMask );
	gx_runtime->debugLog( t.c_str() );
	t="G Mask:"+itobin( pf.dwGBitMask );
	gx_runtime->debugLog( t.c_str() );
	t="B Mask:"+itobin( pf.dwBBitMask );
	gx_runtime->debugLog( t.c_str() );
	t="A Mask:"+itobin( pf.dwRGBAlphaBitMask );
	gx_runtime->debugLog( t.c_str() );
}

static void pickTexFmts( gxGraphics *g,int hi ){
	//texRGBFmt.
	{
		int pick=-1,max=0,bits;
		for( int d=g->primFmt.dwRGBBitCount;d<=32;d+=8 ){
			for( int k=0;k<tex_fmts.size();++k ){
				const TexFmt &t=tex_fmts[k];
				if( t.bits>d || !t.rgb_bits || t.rgb_bits<max ) continue;
				if( t.rgb_bits==max && t.bits>=bits ) continue;
				pick=k;max=t.rgb_bits;bits=t.bits;
			}
			if( !hi && pick>=0 ) break;
		}
		if( pick<0 ) g->texRGBFmt[hi]=g->primFmt;
		else g->texRGBFmt[hi]=tex_fmts[pick].fmt;
	}
	//texAlphaFmt
	{
		int pick=-1,max=0,bits;
		for( int d=g->primFmt.dwRGBBitCount;d<=32;d+=8 ){
			for( int k=0;k<tex_fmts.size();++k ){
				const TexFmt &t=tex_fmts[k];
				if( t.bits>d || !t.a_bits || t.a_bits<max ) continue;
				if( t.a_bits==max && t.bits>=bits ) continue;
				pick=k;max=t.a_bits;bits=t.bits;
			}
			if( !hi && pick>=0 ) break;
		}
		if( pick<0 ) g->texAlphaFmt[hi]=g->primFmt;
		else g->texAlphaFmt[hi]=tex_fmts[pick].fmt;
	}
	//texRGBAlphaFmt
	{
		int pick=-1,a8rgb8=-1,max=0,bits;
		for( int d=g->primFmt.dwRGBBitCount;d<=32;d+=8 ){
			for( int k=0;k<tex_fmts.size();++k ){
				const TexFmt &t=tex_fmts[k];
				if( t.a_bits==8 && t.bits==16 ){ a8rgb8=k;continue; }
				if( t.bits>d || !t.a_bits || !t.rgb_bits || t.a_bits<max ) continue;
				if( t.a_bits==max && t.bits>=bits ) continue;
				pick=k;max=t.a_bits;bits=t.bits;
			}
			if( !hi && pick>=0 ) break;
		}
		if( pick<0 ) pick=a8rgb8;
		if( pick<0 ) g->texRGBAlphaFmt[hi]=g->primFmt;
		else g->texRGBAlphaFmt[hi]=tex_fmts[pick].fmt;
	}
	//texRGBMaskFmt...
	{
		int pick=-1,max=0,bits;
		for( int d=g->primFmt.dwRGBBitCount;d<=32;d+=8 ){
			for( int k=0;k<tex_fmts.size();++k ){
				const TexFmt &t=tex_fmts[k];
				if( !t.a_bits || !t.rgb_bits || t.rgb_bits<max ) continue;
				if( t.rgb_bits==max && t.bits>=bits ) continue;
				pick=k;max=t.rgb_bits;bits=t.bits;
			}
			if( !hi && pick>=0 ) break;
		}
		if( pick<0 ) g->texRGBMaskFmt[hi]=g->primFmt;
		else g->texRGBMaskFmt[hi]=tex_fmts[pick].fmt;
	}
}

gxScene *gxGraphics::createScene( int flags ){
	if( scene_set.size() ) return 0;

	//get d3d
	if( dirDraw->QueryInterface( IID_IDirect3D7,(void**)&dir3d )>=0 ){
		//enum devices
		maxDevType=0;
		if( dir3d->EnumDevices( enumDevice,this )>=0 && maxDevType>1 ){
			//enum zbuffer formats
			zbuffFmt.dwZBufferBitDepth=0;
			if( dir3d->EnumZBufferFormats( dir3dDevDesc.deviceGUID,enumZbuffFormat,this )>=0 ){
				//create zbuff for back buffer
				if( back_canvas->attachZBuffer() ){
					//create 3d device
					if( dir3d->CreateDevice( dir3dDevDesc.deviceGUID,back_canvas->getSurface(),&dir3dDev )>=0 ){
						//enum texture formats
						tex_fmts.clear();
						if( dir3dDev->EnumTextureFormats( enumTextureFormat,this )>=0 ){
							pickTexFmts( this,0 );
							pickTexFmts( this,1 );
							tex_fmts.clear();
#ifdef BETA
							gx_runtime->debugLog( "Texture RGB format:" );
							debugPF( texRGBFmt );
							gx_runtime->debugLog( "Texture Alpha format:" );
							debugPF( texAlphaFmt );
							gx_runtime->debugLog( "Texture RGB Alpha format:" );
							debugPF( texRGBAlphaFmt );
							gx_runtime->debugLog( "Texture RGB Mask format:" );
							debugPF( texRGBMaskFmt );
							gx_runtime->debugLog( "Texture Primary format:" );
							debugPF( primFmt );
							string ts="ZBuffer Bit Depth:"+itoa( zbuffFmt.dwZBufferBitDepth );
							gx_runtime->debugLog( ts.c_str() );
#endif
							gxScene *scene=d_new gxScene( this,back_canvas );
							scene_set.insert( scene );

							dummy_mesh=createMesh( 8,12,0 );

							return scene;
						}
						dir3dDev->Release();
						dir3dDev=0;
					}
					back_canvas->releaseZBuffer();
				}
			}
		}
		dir3d->Release();
		dir3d=0;
	}
	return 0;
}

gxScene *gxGraphics::verifyScene( gxScene *s ){
	return scene_set.count( s ) ? s : 0;
}

void gxGraphics::freeScene( gxScene *scene ){
	if( !scene_set.erase( scene ) ) return;
	dummy_mesh=0;
	while( mesh_set.size() ) freeMesh( *mesh_set.begin() );
	back_canvas->releaseZBuffer();
	if( dir3dDev ){ dir3dDev->Release();dir3dDev=0; }
	if( dir3d ){ dir3d->Release();dir3d=0; }
	delete scene;
}

gxMesh *gxGraphics::createMesh( int max_verts,int max_tris,int flags ){

	static const int VTXFMT=
	D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX2|
	D3DFVF_TEXCOORDSIZE2(0)|D3DFVF_TEXCOORDSIZE2(1);

	int vbflags=0;

	//XP or less?
	if( runtime->osinfo.dwMajorVersion<6 ){
		vbflags|=D3DVBCAPS_WRITEONLY;
	}

	D3DVERTEXBUFFERDESC desc={ sizeof(desc),(DWORD)vbflags,VTXFMT,(DWORD)max_verts };

	IDirect3DVertexBuffer7 *buff;
	if( dir3d->CreateVertexBuffer( &desc,&buff,0 )<0 ) return 0;
	WORD *indices=d_new WORD[max_tris*3];
	gxMesh *mesh=d_new gxMesh( this,buff,indices,max_verts,max_tris );
	mesh_set.insert( mesh );
	return mesh;
}

gxMesh *gxGraphics::verifyMesh( gxMesh *m ){
	return mesh_set.count( m ) ? m : 0;
}

void gxGraphics::freeMesh( gxMesh *mesh ){
	if( mesh_set.erase( mesh ) ) delete mesh;
}
