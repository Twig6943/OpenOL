/**
 * Copyright 2012 Red Barrels, Inc. All Rights Reserved.
 */
class OLBTNode extends Object
	abstract
	native(AI);

var() name NodeName;

var class TaskType;

var  transient int SearchTag;

/** for editor use. */
var editoronly int DrawWidth;
var editoronly int DrawHeight;
var editoronly float NodePosX;
var editoronly float NodePosY;
var editoronly int InDrawY;
var editoronly string CategoryDesc;

cpptext
{
	virtual UOLBTTask* CreateTask();
	virtual void DestroyTask(UOLBTTask* aTask);

	void GetNodes(TArray<UOLBTNode*>& Nodes);
	virtual void GetNodesInternal(TArray<UOLBTNode*>& Nodes);

	/* for editor use. */
	virtual FIntPoint GetConnectionLocation(INT ConnType, INT ConnIndex);
	virtual void DrawNode(FCanvas* Canvas, const TArray<UOLBTNode*>& SelectedNodes);
	virtual FString GetNodeTitle();

	static UBOOL bNodeSearching;
	static INT CurrentSearchTag;
}

defaultproperties
{
	CategoryDesc="Leaf Nodes"
}