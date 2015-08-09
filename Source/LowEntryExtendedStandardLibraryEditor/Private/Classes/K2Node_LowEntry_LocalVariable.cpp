#include "LowEntryExtendedStandardLibraryEditorPrivatePCH.h"


// settings >>
	#include "K2Node_LowEntry_LocalVariable.h"


	static const FString InputPinName = FString(TEXT("Value"));
	static const FString OutputPinName = FString(TEXT("Value "));


	#define MENU_CATEGORY "Low Entry|Extended Standard Library|Utilities|Other"
	#define TITLE "Local Variable"
	#define TOOLTIP "Caches a value."
	#define IS_PURE false
// settings <<


#define LOCTEXT_NAMESPACE "MakeArrayNode"


/////////////////////////////////////////////////////
// FKCHandler_LowEntry_LocalVariable

class FKCHandler_LowEntry_LocalVariable : public FNodeHandlingFunctor
{
public:
	FKCHandler_LowEntry_LocalVariable(FKismetCompilerContext& InCompilerContext) : FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_LowEntry_LocalVariable* CastedNode = CastChecked<UK2Node_LowEntry_LocalVariable>(Node);
		FNodeHandlingFunctor::RegisterNets(Context, Node);

		// Create a local term to drop the object into
		UEdGraphPin* OutputPin = CastedNode->GetOutputPin();
		FBPTerminal* OutputPinTerm = Context.CreateLocalTerminalFromPinAutoChooseScope(OutputPin, Context.NetNameMap->MakeValidName(OutputPin) + TEXT("_Object"));
		OutputPinTerm->bPassedByReference = false;
		OutputPinTerm->Source = Node;
		Context.NetMap.Add(OutputPin, OutputPinTerm);
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_LowEntry_LocalVariable* CastedNode = CastChecked<UK2Node_LowEntry_LocalVariable>(Node);
		UEdGraphPin* InputPin = CastedNode->GetInputPin();
		UEdGraphPin* OutputPin = CastedNode->GetOutputPin();

		FBPTerminal** InputPinVariable = Context.NetMap.Find(FEdGraphUtilities::GetNetFromPin(InputPin));
		FBPTerminal** OutputPinVariable = Context.NetMap.Find(OutputPin);
		check(InputPinVariable);
		check(OutputPinVariable);


		FBlueprintCompiledStatement& AssignStatement = Context.AppendStatementForNode(Node);
		AssignStatement.Type = KCST_Assignment;
		AssignStatement.RHS.Add(*InputPinVariable);
		AssignStatement.LHS = *OutputPinVariable;


		if(!IS_PURE)
		{
			GenerateSimpleThenGoto(Context, *Node);
		}
	}
};


/////////////////////////////////////////////////////
// UK2Node_LowEntry_LocalVariable

UK2Node_LowEntry_LocalVariable::UK2Node_LowEntry_LocalVariable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FNodeHandlingFunctor* UK2Node_LowEntry_LocalVariable::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_LowEntry_LocalVariable(CompilerContext);
}

FText UK2Node_LowEntry_LocalVariable::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TITLE);
}

UEdGraphPin* UK2Node_LowEntry_LocalVariable::GetInputPin() const
{
	return FindPin(InputPinName);
}

UEdGraphPin* UK2Node_LowEntry_LocalVariable::GetOutputPin() const
{
	return FindPin(OutputPinName);
}

bool UK2Node_LowEntry_LocalVariable::IsNodePure() const
{
	return IS_PURE;
}

void UK2Node_LowEntry_LocalVariable::AllocateDefaultPins()
{
	if(!IS_PURE)
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, UEdGraphSchema_K2::PN_Execute);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, TEXT(""), NULL, false, false, UEdGraphSchema_K2::PN_Then);
	}

	UEdGraphPin* InputPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, TEXT(""), NULL, false, false, *InputPinName);
	UEdGraphPin* OutputPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, TEXT(""), NULL, false, false, *OutputPinName);
}

void UK2Node_LowEntry_LocalVariable::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if(Pin != nullptr)
	{
		if((Pin->Direction == EGPD_Output) || (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec))
		{
			return;
		}

		PropagatePinType();
	}
}

void UK2Node_LowEntry_LocalVariable::PropagatePinType()
{
	UEdGraphPin* InputPin = GetInputPin();
	UEdGraphPin* OutputPin = GetOutputPin();

	if(InputPin->LinkedTo.Num() > 0)
	{
		InputPin->PinType = InputPin->LinkedTo[0]->PinType;
	}
	else
	{
		InputPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
		InputPin->PinType.PinSubCategory = TEXT("");
		InputPin->PinType.PinSubCategoryObject = NULL;
	}

	if(!ArePinTypesEqual(InputPin->PinType, OutputPin->PinType))
	{
		OutputPin->PinType = InputPin->PinType;
		if((OutputPin->GetOwningNode() != NULL) && (OutputPin->GetOwningNode()->GetGraph() != NULL))
		{
			OutputPin->GetOwningNode()->GetGraph()->NotifyGraphChanged();
		}
	}
}

bool UK2Node_LowEntry_LocalVariable::ArePinTypesEqual(const FEdGraphPinType& A, const FEdGraphPinType& B)
{
	if(A.PinCategory != B.PinCategory)
	{
		return false;
	}
	if(A.PinSubCategory != B.PinSubCategory)
	{
		return false;
	}
	if(A.PinSubCategoryObject != B.PinSubCategoryObject)
	{
		return false;
	}
	if(A.bIsArray != B.bIsArray)
	{
		return false;
	}
	if(A.bIsConst != B.bIsConst)
	{
		return false;
	}
	if(A.bIsReference != B.bIsReference)
	{
		return false;
	}
	if(A.bIsWeakPointer != B.bIsWeakPointer)
	{
		return false;
	}
	return true;
}

void UK2Node_LowEntry_LocalVariable::PostReconstructNode()
{
	PropagatePinType();
}

FText UK2Node_LowEntry_LocalVariable::GetTooltipText() const
{
	return FText::FromString(TOOLTIP);
}

bool UK2Node_LowEntry_LocalVariable::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	auto Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	if(!ensure(Schema) || (ensure(OtherPin) && Schema->IsExecPin(*OtherPin) && !Schema->IsExecPin(*MyPin)))
	{
		OutReason = NSLOCTEXT("K2Node", "MakeArray_InputIsExec", "Doesn't allow execution input!").ToString();
		return true;
	}

	return false;
}

void UK2Node_LowEntry_LocalVariable::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	auto Schema = Cast<const UEdGraphSchema_K2>(GetSchema());
	auto OutputPin = GetOutputPin();
	if(!ensure(Schema) || !ensure(OutputPin) || Schema->IsExecPin(*OutputPin))
	{
		MessageLog.Error(*NSLOCTEXT("K2Node", "MakeArray_OutputIsExec", "Uaccepted type in @@").ToString(), this);
	}
}

void UK2Node_LowEntry_LocalVariable::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if(ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_LowEntry_LocalVariable::GetMenuCategory() const
{
	return FText::FromString(TEXT(MENU_CATEGORY));
}

#undef LOCTEXT_NAMESPACE
