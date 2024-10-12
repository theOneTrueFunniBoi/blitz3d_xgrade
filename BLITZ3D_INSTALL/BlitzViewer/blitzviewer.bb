
;Blitz media viewer.
;
;Create executable in 'bin'

Global mouse_left_limit% = 250, mouse_right_limit% = GraphicsWidth () - 250
Global mouse_top_limit% = 150, mouse_bottom_limit% = GraphicsHeight () - 150
Global MXS#=180.0,MYS#=0.0
Global Faster% = False
Global Slower% = False

AppTitle "Blitz3D Media Viewer: Initializing"
Global fil$=Lower$( CommandLine$() )

Global texA%[255]

Global index%=Instr( fil$,"." )
Global ext$

Graphics3D 800,600,0,2

If (index>0) Then ext$=Mid$( fil$,index+1 )
Select ext$
	Case "x","3ds","b3d"
		AppTitle "Blitz3D Media Viewer: "+CommandLine$()
		ShowModel( fil$,False,False )
	Case "md2"
		AppTitle "Blitz3D Media Viewer: "+CommandLine$()
		ShowModel( fil$,True,False )	
	Case "rmesh"
		AppTitle "Blitz3D Media Viewer: "+CommandLine$()
		ShowModel( fil$,False,True )
	Case "bmp","jpg","png","pcx","tga","iff"
		AppTitle "Blitz3D Media Viewer: "+CommandLine$()
		ShowImage( fil$ )
	Case "wav","ogg"
		AppTitle "Blitz3D Media Viewer: "+CommandLine$()
		ShowSound( fil$ )
	Case "mp3","mid","mod","x3m","xm","it"
		AppTitle "Blitz3D Media Viewer: "+CommandLine$()
		ShowMusic( fil$ )
	Default
		RuntimeError ("Invalid File Extension: '"+ext+"'")
End Select

Function ShowModel( fil$,md2%,rmesh% )
	Local model%
	If md2
		model=LoadMD2( fil$ )
		If model Then ScaleEntity model,.025,.025,.025
	ElseIf rmesh
		model=LoadRMesh( fil$ )
	Else
		model=LoadMesh( fil$ )
		If model Then FitMesh model,-1,-1,-1,2,2,2,True
	EndIf
	If model=0 Then RuntimeError "Unable to load 3D mesh:"+fil$
	Local sc%=CountSurfaces(model)
	Local vc%,tc%
	For k%=1 To sc
		vc=vc+CountVertices( GetSurface( model,k ) )
		tc=tc+CountTriangles( GetSurface( model,k ) )
	Next
	Local camera%=CreateCamera()
	CameraClsColor camera,0,0,64
	CameraRange camera,.01,20
	Local light%=CreateLight()
	TurnEntity light,45,45,0	
	;MoveMouse GraphicsWidth()/2,GraphicsHeight()/2
	Repeat	
		Cls()
		HidePointer()
		
		If (MouseX() > mouse_right_limit) Or (MouseX() < mouse_left_limit) Or (MouseY() > mouse_bottom_limit) Or (MouseY() < mouse_top_limit)
			MoveMouse GraphicsWidth()/2,GraphicsHeight()/2
		EndIf
		
		MXS# = MXS# + MouseXSpeed()*0.25
		MYS# = MYS# + MouseYSpeed()*0.25
		
		MYS = Max(Min(MYS,85),-85)
		
		RotateEntity camera,MYS,-MXS,0
		
		Faster = 0
		If KeyDown(42) Then Faster = 1
		Slower = 0
		If KeyDown(29) Then Slower = 1
		
		If KeyDown(17) Then MoveEntity camera,0,0,((0.05)+(0.05*Faster))/(1+Slower)
		If KeyDown(30) Then MoveEntity camera,(-0.05-(0.05*Faster))/(1+Slower),0,0
		If KeyDown(31) Then MoveEntity camera,0,0,(-0.05-(0.05*Faster))/(1+Slower)
		If KeyDown(32) Then MoveEntity camera,(0.05+(0.05*Faster))/(1+Slower),0,0
		
		If KeyDown(1) Then End
		
		UpdateWorld
		RenderWorld
		Text 0,0,"Triangles:"+tc+" Vertices:"+vc+" Surfaces:"+sc		
		Text 0,15,"CameraRotX: "+MXS+" CameraRotY: "+(-MYS)
		Flip
	Forever
End Function

Function Min#(a#,b#)
	If a < b Then Return a Else Return b
End Function

Function Max#(a#,b#)
	If a > b Then Return a Else Return b
End Function

Function LoadRMesh(file$)
	;generate a texture made of white
	Local blankTexture%
	blankTexture=CreateTexture(4,4,1,1)
	ClsColor 255,255,255
	SetBuffer TextureBuffer(blankTexture)
	Cls
	SetBuffer BackBuffer()
	
	Local pinkTexture%
	pinkTexture=CreateTexture(4,4,1,1)
	ClsColor 255,255,255
	SetBuffer TextureBuffer(pinkTexture)
	Cls
	SetBuffer BackBuffer()
	
	ClsColor 0,0,0
	
	;read the file
	Local f%=ReadFile(file)
	Local i%,j%,k%,x#,y#,z#,yaw#
	Local vertex%
	Local temp1i%,temp2i%,temp3i%
	Local temp1#,temp2#,temp3#
	Local temp1s$, temp2s$
	
	Local collisionMeshes% = CreatePivot()
	
	Local hasTriggerBox% = False
	
	For i=0 To 3 ;reattempt up to 3 times
		If f=0 Then
			f=ReadFile(file)
		Else
			Exit
		EndIf
	Next
	If f=0 Then RuntimeError "Error reading file "+Chr(34)+file+Chr(34)
	Local isRMesh$ = ReadString(f)
	If isRMesh$="RoomMesh"
		;Continue
	ElseIf isRMesh$="RoomMesh.HasTriggerBox"
		hasTriggerBox% = True
	Else
		RuntimeError Chr(34)+file+Chr(34)+" is Not RMESH ("+isRMesh+")"
	EndIf
	
	file=StripFilename(file)
	
	Local count%,count2%
	
	;drawn meshes
	Local Opaque%,Alpha%
	
	Opaque=CreateMesh()
	Alpha=CreateMesh()
	
	count = ReadInt(f)
	Local childMesh%
	Local surf%,brush%
	
	Local isAlpha%
	
	Local u#,v#
	
	For i=1 To count ;drawn mesh
		childMesh=CreateMesh()
		
		surf=CreateSurface(childMesh)
		
		brush=CreateBrush()
		
		texA[0]=0 : texA[1]=0
		
		isAlpha=0
		For j=0 To 1
			temp1i=ReadByte(f)
			If temp1i<>0 Then
				temp1s=ReadString(f)
				texA[j]=GetTextureFromCache(temp1s)
				If texA[j]=0 Then ;texture is not in cache
					Select True
						Case temp1i<3
							texA[j]=LoadTexture(file+temp1s,1)
						Default
							texA[j]=LoadTexture(file+temp1s,3)
					End Select
					
					If texA[j]<>0 Then
						If temp1i=1 Then TextureBlend texA[j],5
						;If Instr(Lower(temp1s),"_lm")<>0 Then
						;	TextureBlend tex[j],3
						;EndIf
						AddTextureToCache(texA[j])
					EndIf
					
				EndIf
				If texA[j]<>0 Then
					isAlpha=2
					If temp1i=3 Then isAlpha=1
					
					TextureCoords texA[j],1-j
				EndIf
			EndIf
		Next
		
		If isAlpha=1 Then
			If texA[1]<>0 Then
				TextureBlend texA[1],2
				BrushTexture brush,texA[1],0,0
			Else
				BrushTexture brush,blankTexture,0,0
			EndIf
		Else
			

			For j=0 To 1
				If texA[j]<>0 Then
					BrushTexture brush,texA[j],0,j+1
				Else
					BrushTexture brush,blankTexture,0,j+1
				EndIf
			Next

		EndIf
		
		surf=CreateSurface(childMesh)
		
		If isAlpha>0 Then PaintSurface surf,brush
		
		FreeBrush brush : brush = 0
		
		count2=ReadInt(f) ;vertices
		
		For j%=1 To count2
			;world coords
			x=ReadFloat(f) : y=ReadFloat(f) : z=ReadFloat(f)
			vertex=AddVertex(surf,x,y,z)
			
			;texture coords
			For k%=0 To 1
				u=ReadFloat(f) : v=ReadFloat(f)
				VertexTexCoords surf,vertex,u,v,0.0,k
			Next
			
			;colors
			temp1i=ReadByte(f)
			temp2i=ReadByte(f)
			temp3i=ReadByte(f)
			VertexColor surf,vertex,temp1i,temp2i,temp3i,1.0
		Next
		
		count2=ReadInt(f) ;polys
		For j%=1 To count2
			temp1i = ReadInt(f) : temp2i = ReadInt(f) : temp3i = ReadInt(f)
			AddTriangle(surf,temp1i,temp2i,temp3i)
		Next
		
		If isAlpha=1 Then
			AddMesh childMesh,Alpha
			EntityAlpha childMesh,0.0
		Else
			AddMesh childMesh,Opaque
			EntityParent childMesh,collisionMeshes
			EntityAlpha childMesh,0.0
			EntityType childMesh,HIT_MAP
			EntityPickMode childMesh,2
			
			;make collision double-sided
			Local flipChild% = CopyMesh(childMesh)
			FlipMesh(flipChild)
			AddMesh flipChild,childMesh
			FreeEntity flipChild			
		EndIf
		
		
	Next
	
	Local hiddenMesh%
	hiddenMesh=CreateMesh()
	
	count=ReadInt(f) ;invisible collision mesh
	For i%=1 To count
		;surf=CreateSurface(hiddenMesh)
		count2=ReadInt(f) ;vertices
		For j%=1 To count2
			;world coords
			x=ReadFloat(f) : y=ReadFloat(f) : z=ReadFloat(f)
			;vertex=AddVertex(surf,x,y,z)
		Next
		
		count2=ReadInt(f) ;polys
		For j%=1 To count2
			temp1i = ReadInt(f) : temp2i = ReadInt(f) : temp3i = ReadInt(f)
			;AddTriangle(surf,temp1i,temp2i,temp3i)
			;AddTriangle(surf,temp1i,temp3i,temp2i)
		Next
	Next
	
	;trigger boxes
	If hasTriggerBox
		DebugLog "TriggerBoxEnable"
		Local tmp% = ReadInt(f)
		Local tmp2%
		For tmp2% = 0 To tmp%-1
			DebugLog "0 "+tmp2
			;rt\TempTriggerbox[tb] = CreateMesh(rt\obj)
			count = ReadInt(f)
			For i%=1 To count
				DebugLog "1 "+i
				;surf=CreateSurface(rt\TempTriggerbox[tb])
				count2=ReadInt(f)
				For j%=1 To count2
					DebugLog "2 "+j
					x=ReadFloat(f) : y=ReadFloat(f) : z=ReadFloat(f)
					;vertex=AddVertex(surf,x,y,z)
				Next
				count2=ReadInt(f)
				For j%=1 To count2
					DebugLog "3 "+j
					temp1i = ReadInt(f) : temp2i = ReadInt(f) : temp3i = ReadInt(f)
					;AddTriangle(surf,temp1i,temp2i,temp3i)
					;AddTriangle(surf,temp1i,temp3i,temp2i)
				Next
			Next
			;rt\TempTriggerboxName[tb] = ReadString(f)
			DebugLog "4"
			;Local tmpS$ = ReadString(f)
			ReadString(f)
		Next
	EndIf
	
	Local rRoomScale# = 8.0 / 2048.0
	
	count=ReadInt(f) ;point entities
	For i%=1 To count
		temp1s=ReadString(f)
		Select temp1s
			Case "screen"
				
				temp1=ReadFloat(f)
				temp2=ReadFloat(f)
				temp3=ReadFloat(f)
				
				temp2s$ =ReadString(f)
				
			Case "waypoint"
				
				temp1=ReadFloat(f)
				temp2=ReadFloat(f)
				temp3=ReadFloat(f)
				
			Case "light"
				
				temp1=ReadFloat(f)
				temp2=ReadFloat(f)
				temp3=ReadFloat(f)
				
				ReadFloat(f) : ReadString(f) : ReadFloat(f)
				
			Case "spotlight"
				
				temp1=ReadFloat(f)
				temp2=ReadFloat(f)
				temp3=ReadFloat(f)
				
				ReadFloat(f) : ReadString(f) : ReadFloat(f) : ReadString(f) : ReadInt(f) : ReadInt(f)
				
			Case "soundemitter"
				
				temp1i=0
				
				/*				
				For j = 0 To MaxRoomEmitters-1
					If rt\TempSoundEmitter[j]=0 Then
						rt\TempSoundEmitterX[j]=ReadFloat(f)
						rt\TempSoundEmitterY[j]=ReadFloat(f)
						rt\TempSoundEmitterZ[j]=ReadFloat(f)
						rt\TempSoundEmitter[j]=ReadInt(f)
						
						rt\TempSoundEmitterRange[j]=ReadFloat(f)
						temp1i=1
						Exit
					EndIf
				Next				
				*/
				
				;If temp1i=0 Then
					ReadFloat(f)
					ReadFloat(f)
					ReadFloat(f)
					ReadInt(f)
					ReadFloat(f)
				;EndIf
				
			Case "playerstart"
				
				temp1=ReadFloat(f) : temp2=ReadFloat(f) : temp3=ReadFloat(f)
				
				ReadString(f)
			Case "model"			
				DebugLog ("model")				
				Local file2$ = ReadString(f)
				If file2<>""			
					Text 5,5,file+"Props\"+file2
					Local model = LoadMesh(file+"Props\"+file2)
					
					;try again
					If (Not model) Then model = LoadMesh(file+"\Props\"+file2)
					
					DebugLog "Attempted To Init Prob Obj: '"+file+"\Props\"+file2+"'."
					
					If (Not model) Then RuntimeError("PropObject: '"+file+"\Props\"+file2+"' faiiled To load.")
					
					temp1=ReadFloat(f) : temp2=ReadFloat(f) : temp3=ReadFloat(f)
					PositionEntity model,temp1/100,temp2/100,temp3/100
					
					temp1=ReadFloat(f) : temp2=ReadFloat(f) : temp3=ReadFloat(f)
					RotateEntity model,temp1,temp2,temp3
					
					temp1=ReadFloat(f) : temp2=ReadFloat(f) : temp3=ReadFloat(f)
					;ScaleEntity model,temp1*rRoomScale,temp2*rRoomScale,temp3*rRoomScale
					ScaleEntity model,temp1/100,temp2/100,temp3/100
					
					;EntityParent model,Opaque
					;EntityType model,HIT_MAP
					;EntityPickMode model,2
				Else
					DebugLog "file = 0"
					temp1=ReadFloat(f) : temp2=ReadFloat(f) : temp3=ReadFloat(f)
					DebugLog temp1+", "+temp2+", "+temp3
					
					;Stop
				EndIf				
			Default			
			RuntimeError("UNSUPPORTED RMesh ENTITY TYPE")
		End Select
	Next
	
	Local obj%
	
	temp1i=CopyMesh(Alpha)
	FlipMesh temp1i
	AddMesh temp1i,Alpha
	FreeEntity temp1i
	
	If brush <> 0 Then FreeBrush brush
	
	AddMesh Alpha,Opaque
	FreeEntity Alpha
	
	EntityFX Opaque,3
	
	EntityAlpha Opaque,1.0

	FreeTexture blankTexture		
	
	ScaleEntity Opaque,EntityScaleX(Opaque)/100,EntityScaleY(Opaque)/100,EntityScaleZ(Opaque)/100
	
	CloseFile f
	
	Return Opaque
End Function

Function EntityScaleX#(entity%, globl% = False)
	If globl Then TFormVector 1, 0, 0, entity, 0 Else TFormVector 1, 0, 0, entity, GetParent(entity)
	Return Sqr(TFormedX() * TFormedX() + TFormedY() * TFormedY() + TFormedZ() * TFormedZ())
End Function 

Function EntityScaleY#(entity%, globl% = False)
	If globl Then TFormVector 0, 1, 0, entity, 0 Else TFormVector 0, 1, 0, entity, GetParent(entity)
	Return Sqr(TFormedX() * TFormedX() + TFormedY() * TFormedY() + TFormedZ() * TFormedZ())
End Function 

Function EntityScaleZ#(entity%, globl% = False)
	If globl Then TFormVector 0, 0, 1, entity, 0 Else TFormVector 0, 0, 1, entity, GetParent(entity)
	Return Sqr(TFormedX() * TFormedX() + TFormedY() * TFormedY() + TFormedZ() * TFormedZ())
End Function 

Function StripFilename$(file$)
	Local mi$=""
	Local lastSlash%=0
	If Len(file)>0
		For i%=1 To Len(file)
			mi=Mid(file$,i,1)
			If mi="\" Or mi="/" Then
				lastSlash=i
			EndIf
		Next
	EndIf
	
	Return Left(file,lastSlash)
End Function

Function StripPath$(file$) 
	Local name$=""
	If Len(file$)>0 
		For i=Len(file$) To 1 Step -1 
			
			mi$=Mid$(file$,i,1) 
			If mi$="\" Or mi$="/" Then Return name$
			
			name$=mi$+name$ 
		Next 
		
	EndIf 
	
	Return name$ 
End Function

Type Materials
	Field name$
	Field Diff
	Field Bump
	
	Field StepSound%
End Type

Function GetTextureFromCache%(name$)
	For tc.Materials=Each Materials
		If tc\name = name Then Return tc\Diff
	Next
	Return 0
End Function

Function GetCache.Materials(name$)
	For tc.Materials=Each Materials
		If tc\name = name Then Return tc
	Next
	Return Null
End Function

Function AddTextureToCache(texture%)
	Local tc.Materials=GetCache(StripPath(TextureName(texture)))
	If tc.Materials=Null Then
		tc.Materials=New Materials
		tc\name=StripPath(TextureName(texture))
		tc\Diff=0
	EndIf
	If tc\Diff=0 Then tc\Diff=texture
End Function

Function ShowImage( fil$ )
	;Graphics 400,300,0,2
	SetBuffer BackBuffer()
	Local image%=LoadImage( fil$ )
	If image=0 RuntimeError "Unable to load image:"+fil$
	MidHandle image
	Local x%=200,y%=150,t%=4
	Repeat
		Cls
		DrawImage image,x,y
		Flip
		Local key%=False
		Repeat
			If KeyHit(1) End
			If KeyDown(200) y=y-t:key=True
			If KeyDown(208) y=y+t:key=True
			If KeyDown(203) x=x-t:key=True
			If KeyDown(205) x=x+t:key=True
			If Not key WaitKey
		Until key
	Forever
End Function

Function ShowSound( fil$ )
	Local sound%=LoadSound( fil$ )
	If sound=0 RuntimeError "Unable to load sound:"+fil$
	Repeat
		PlaySound sound
		WaitKey
		If KeyHit(1) End
	Forever
End Function

Function ShowMusic( fil$ )
	Local music%=PlayMusic( fil$ )
	If music=0 RuntimeError "Unable to play music: "+fil$
	WaitKey
End Function
;~IDEal Editor Parameters:
;~C#Blitz3D