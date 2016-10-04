// space before colon
// could not find text section - change message
#include <iostream>
#include <cmath>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
using namespace std;
struct LabelTable
{
	string label;
	int address;
};
struct MemoryElement
{
	string label;
	int value;
};
class MIPSSimulator
{
	private:
		string Registers[32];
		int RegisterValues[32];
		string InstructionSet[17];
		int Mode;
		vector<string> InputProgram;
		int NumberOfInstructions;
		string current_instruction;
		int ProgramCounter;
		int MaxLength;
		int halt_value;
		int r[3];
		vector<struct LabelTable> TableOfLabels;
		vector<struct MemoryElement> Memory;
		int Stack[100];
		void add();
		void addi();
		void sub();
		void mul();
		void andf();
		void andi();
		void orf();
		void ori();
		void nor();
		void slt();
		void slti();
		void lw();
		void sw();
		void beq();
		void bne();
		void j();
		void halt();
		void preprocess();
		void ReadInstruction(int line);
		int ParseInstruction();
		void ReportError();
		void ExecuteInstruction(int instruction);
		void OnlySpaces(int lower, int upper, string str);
		void RemoveSpaces(string &str);
		void assertNumber(string str);
		void findRegister(int number);
		string findLabel();
		void assertRemoveComma();
		void checkStackBounds(int index);

	public:
		MIPSSimulator(int mode, string fileName);
		void Execute();
		void displayState();
};
int sorttable(LabelTable a, LabelTable b);
int sortmemory(MemoryElement a, MemoryElement b);
void MIPSSimulator::Execute()
{
	preprocess();
	while(ProgramCounter<NumberOfInstructions && halt_value==0)
	{
		ReadInstruction(ProgramCounter);
		RemoveSpaces(current_instruction);
		if(current_instruction=="")
		{
			ProgramCounter++;
			continue;
		}
		int instruction=ParseInstruction();
		ExecuteInstruction(instruction);
		if(instruction<13 || instruction>15)
		{
			ProgramCounter++;
		}
		if(Mode==0)
		{
			displayState();
			getchar();
		}
	}
	displayState();
	if(halt_value==0)
	{
		cout<<"Error: Program ended without halt"<<endl;
		exit(1);
	}
}
MIPSSimulator::MIPSSimulator(int mode, string fileName)
{
	MaxLength=10000;
	NumberOfInstructions=0;
	ProgramCounter=0;
	halt_value=0;
	Memory.clear();
	TableOfLabels.clear();
	string tempRegisters[]={"zero","at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3","t4","t5","t6",
	"t7","s0","s1","s2","s3","s4","s5","s6","s7","t8","t9","k0","k1","gp","sp","s8","ra"};
	for(int i=0;i<32;i++)
	{
		Registers[i]=tempRegisters[i];
	}
	for(int i=0;i<32;i++)
	{
		RegisterValues[i]=0;
	}
	string tempInstructionSet[]={"add","sub","mul","and","or","nor","slt","addi","andi","ori","slti","lw",
	"sw","beq","bne","j","halt"};
	for(int i=0;i<17;i++)
	{
		InstructionSet[i]=tempInstructionSet[i];
	}
	for(int i=0;i<100;i++)
	{
		Stack[i]=0;
	}
	RegisterValues[29]=40396;
	Mode=mode;
	ifstream InputFile;
	InputFile.open(fileName.c_str(),ios::in);
	if(!InputFile)
	{
		cout<<"Error: File does not exist or could not be opened"<<endl;
		exit(1);
	}
	string tempString;
	while(getline(InputFile,tempString))
	{
		NumberOfInstructions++;
		if(NumberOfInstructions>MaxLength)
		{
			cout<<"Error: Number of instructions in input too large, maximum allowed is "<<MaxLength<<" instructions"<<endl;
			exit(1);
		}
		InputProgram.push_back(tempString);
	}
	InputFile.close();
}
void MIPSSimulator::preprocess()
{
	int i=0,j=0;
	int current_section=-1;
	int index;
	int commentindex;
	int flag=0;
	string tempString="";
	int	isLabel=0;
	int	doneFlag=0;
	int dataStart=0;
	int textStart=0;
	for(i=0;i<NumberOfInstructions;i++)
	{
		ReadInstruction(i);
		if(current_instruction=="")
		{
			continue;
		}
		index=current_instruction.find(".data");
		if(index==-1)
		{
			continue;
		}
		else if(flag==0)
		{
			flag=1;
			OnlySpaces(0,index,current_instruction);
			OnlySpaces(index+5,current_instruction.size(),current_instruction);
			current_section=0;
			dataStart=i;
		}
		else if(flag==1)
		{
			cout<<"Error: Multiple instances of .data"<<endl;
			ReportError();
		}
	}
	int LabelIndex;
	if(current_section==0)
	{
		for(i=dataStart+1;i<NumberOfInstructions;i++)
		{
			ReadInstruction(i);
			RemoveSpaces(current_instruction);
			if(current_instruction=="")
			{
				continue;
			}
			LabelIndex=current_instruction.find(":");
			if(LabelIndex==-1)
			{
				if(current_instruction.find(".text")==-1)
				{
					cout<<"Error: Unexpected symbol in data section"<<endl;
					ReportError();
				}
				else
				{
					break;
				}
			}
			if(LabelIndex==0)
			{
				cout<<"Error: Label name expected"<<endl;
				ReportError();
			}
			tempString="";
			int isLabel=0;
			int doneFlag=0;
			for(j=LabelIndex-1;j>=0;j--)
			{
				if(current_instruction[j]!=' ' && current_instruction[j]!='\t' && doneFlag==0)
				{
					isLabel=1;
					tempString=current_instruction[j]+tempString;
				}
				else if(current_instruction[j]!=' ' && current_instruction[j]!='\t' && doneFlag==1)
				{
					cout<<"Error: Unexpected text before label name"<<endl;
					ReportError();
				}
				else if(isLabel==0)
				{
					cout<<"Error: Label name expected"<<endl;
					ReportError();
				}
				else
				{
					doneFlag=1;
				}

			}//TODO check if label has invalid characters
			MemoryElement tempMemory;
			tempMemory.label=tempString;
			int wordIndex=current_instruction.find(".word"); //only .word supported as of now
			if(wordIndex==-1)
			{
				cout<<"Error: .word not found"<<endl;
				ReportError();
			}
			OnlySpaces(LabelIndex+1,wordIndex,current_instruction);
			int foundValue=0;
			int doneFinding=0;
			tempString="";
			for(j=wordIndex+5;j<current_instruction.size();j++)
			{
				if(foundValue==1 && (current_instruction[j]==' ' || current_instruction[j]=='\t') && doneFinding==0)
				{
					doneFinding=1;
				}
				else if(foundValue==1 && current_instruction[j]!=' ' && current_instruction[j]!='\t' && doneFinding==1)
				{
					cout<<"Error: Unexpected text after value"<<endl;
					ReportError();
				}
				else if(foundValue==0 && current_instruction[j]!=' ' && current_instruction[j]!='\t')
				{
					foundValue=1;
					tempString=tempString+current_instruction[j];
				}
				else if(foundValue==1 && current_instruction[j]!=' ' && current_instruction[j]!='\t')
				{
					tempString=tempString+current_instruction[j];
				}
			}
			assertNumber(tempString);
			tempMemory.value=stoi(tempString);
	 		Memory.push_back(tempMemory);
		}
	}
	sort(Memory.begin(),Memory.end(),sortmemory);
	for(i=0;Memory.size()>0 && i<Memory.size()-1;i++)
	{
		if(Memory[i].label==Memory[i+1].label)
		{
			cout<<"Error: One or more labels are repeated"<<endl;
			exit(1);
		}
	}
	int textFlag=0;
	int textIndex=0;
	for(i=ProgramCounter;i<NumberOfInstructions;i++)
	{
		ReadInstruction(i);
		if(current_instruction=="")
		{
			continue;
		}
		textIndex=current_instruction.find(".text");
		if(textIndex==-1)
		{
			continue;
		}
		else if(textFlag==0)
		{
			textFlag=1;
			OnlySpaces(0,textIndex,current_instruction);
			OnlySpaces(textIndex+5,current_instruction.size(),current_instruction);
			current_section=1;
			textStart=i;
		}
		else if(textFlag==1)
		{
			cout<<"Error: Multiple instances of .text"<<endl;
			ReportError();
		}
	}
	if(current_section!=1)
	{
		cout<<"Error: Could not find text section"<<endl;
		exit(1);
	}
	int MainIndex=0;
	int foundMain=0;
	LabelIndex=0;
	for(i=textStart+1;i<NumberOfInstructions;i++)
	{
		ReadInstruction(i);
		if(current_instruction=="")
		{
			continue;
		}
		LabelIndex=current_instruction.find(":");
		if(LabelIndex==0)
		{
			cout<<"Error: Label name expected"<<endl;
			ReportError();
		}
		if(LabelIndex==-1)
		{
			continue;
		}
		tempString="";
		isLabel=0;
		doneFlag=0;
		for(j=LabelIndex-1;j>=0;j--)
		{
			if(current_instruction[j]!=' ' && current_instruction[j]!='\t' && doneFlag==0)
			{
				isLabel=1;
				tempString=current_instruction[j]+tempString;
			}
			else if(current_instruction[j]!=' ' && current_instruction[j]!='\t' && doneFlag==1)
			{
				cout<<"Error: Unexpected text before label name"<<endl;
				ReportError();
			}
			else if(isLabel==0)
			{
				cout<<"Error: Label name expected"<<endl;
				ReportError();
			}
			else
			{
				doneFlag=1;
			}
		}//check if label has invalid characters to be done
		//check if anything after label
		if(tempString=="main")
		{
			foundMain=1;
			MainIndex=ProgramCounter+1;
		}
		else
		{
			LabelTable tempLabel;
			tempLabel.address=ProgramCounter;
			tempLabel.label=tempString;
			TableOfLabels.push_back(tempLabel);
		}
	}
	sort(TableOfLabels.begin(),TableOfLabels.end(),sorttable);
	for(i=0;TableOfLabels.size()>0 && i<(TableOfLabels.size()-1);i++)
	{
		if(TableOfLabels[i].label==TableOfLabels[i+1].label)
		{
			cout<<"Error: One or more labels are repeated"<<endl;
			exit(1);
		}
	}
	if(foundMain==0)
	{
		cout<<"Error: Could not find main"<<endl;
		exit(1);
	}
	ProgramCounter=MainIndex;
	cout<<"Initialized and ready to execute. Current state is as follows:"<<endl;
	displayState();
	cout<<endl<<"Starting execution"<<endl<<endl;
}
void MIPSSimulator::ReportError()
{
	cout<<"Error found in instruction at address: "<<(4*ProgramCounter)<<": "<<InputProgram[ProgramCounter]<<endl;
	displayState();
	exit(1);
}
void MIPSSimulator::ReadInstruction(int line)
{
	current_instruction=InputProgram[line];
	if(current_instruction.find("#")!=-1)
	{
		current_instruction=current_instruction.substr(0,current_instruction.find("#"));
	}
	ProgramCounter=line;
}
int MIPSSimulator::ParseInstruction()
{
	int i=0,j=0;
	RemoveSpaces(current_instruction);
	if(current_instruction.find(":")!=-1)
	{
		return -1;
	}
	if(current_instruction.size()<4)
	{
		cout<<"Error: Unknown operation"<<endl;
		ReportError();
	}
	for(j=0;j<4;j++)
	{
		if(current_instruction[j]==' ' || current_instruction[j]=='\t')
		{
			break;
		}
	}
	string operation=current_instruction.substr(0,j);
	if(current_instruction.size()>0 && j<current_instruction.size()-1)
	{
		current_instruction=current_instruction.substr(j+1);
	}
	int foundOp=0;
	int OperationID=-1;
	for(i=0;i<17;i++)
	{
		if(operation==InstructionSet[i])
		{
			OperationID=i;
			break;
		}
	}
	if(OperationID==-1)
	{
		cout<<"Error: Unknown operation"<<endl;
		ReportError();
	}
	if(OperationID<7)
	{
		for(int count=0;count<3;count++)
		{
			RemoveSpaces(current_instruction);
			findRegister(count);
			RemoveSpaces(current_instruction);
			if(count==2)
			{
				break;
			}
			assertRemoveComma();
		}
		if(current_instruction!="")
		{
			cout<<"Error: Too few arguments provided"<<endl;
			ReportError();
		}
	}
	else if(OperationID<11)
	{
		for(int count=0;count<2;count++)
		{
			RemoveSpaces(current_instruction);
			findRegister(count);
			RemoveSpaces(current_instruction);
			assertRemoveComma();
		}
		RemoveSpaces(current_instruction);
		string tempString=findLabel();
		assertNumber(tempString);
		r[2]=stoi(tempString);
	}
	else if(OperationID<13)
	{
		string tempString="";
		int offset;
		RemoveSpaces(current_instruction);
		findRegister(0);
		RemoveSpaces(current_instruction);
		assertRemoveComma();
		RemoveSpaces(current_instruction);
		if((current_instruction[0]>47 && current_instruction[0]<58) || current_instruction[0]=='-')
		{
			j=0;
			while(j<current_instruction.size() && current_instruction[j]!=' ' && current_instruction[j]!='\t' && current_instruction[j]!='(')
			{
				tempString=tempString+current_instruction[j];
				j++;
			}
			if(j==current_instruction.size())
			{
				cout<<"Error: '(' expected"<<endl;
				ReportError();
			}
			assertNumber(tempString);
			offset=stoi(tempString);
			current_instruction=current_instruction.substr(j);
			RemoveSpaces(current_instruction);
			if(current_instruction=="" || current_instruction[0]!='(' || current_instruction.size()<2)
			{
				cout<<"Error: '(' expected"<<endl;
				ReportError();
			}
			current_instruction=current_instruction.substr(1);
			RemoveSpaces(current_instruction);
			findRegister(1);
			RemoveSpaces(current_instruction);
			if(current_instruction=="" || current_instruction[0]!=')')
			{
				cout<<"Error: ')' expected"<<endl;
				ReportError();
			}
			current_instruction=current_instruction.substr(1);
			OnlySpaces(0,current_instruction.size(),current_instruction);
			r[2]=offset;
		}
		else
		{
			tempString=findLabel();
			int foundLocation=0;
			for(j=0;j<Memory.size();j++)
			{

				if(tempString==Memory[j].label)
				{
					foundLocation=1;
					if(OperationID==11)
					{
						r[1]=Memory[j].value;
					}
					else
					{
						r[1]=j;
					}
					break;
				}
			}
			if(foundLocation==0)
			{
				cout<<"Error: Invalid label"<<endl;
				ReportError();
			}
			r[2]=-1;
		}
	}
	else if(OperationID<15)
	{
		for(int count=0;count<2;count++)
		{
			RemoveSpaces(current_instruction);
			findRegister(count);
			RemoveSpaces(current_instruction);
			assertRemoveComma();
		}
		RemoveSpaces(current_instruction);
		string tempString=findLabel();
		int foundAddress=0;
		for(j=0;j<TableOfLabels.size();j++)
		{
			if(tempString==TableOfLabels[j].label)
			{
				foundAddress=1;
				r[2]=TableOfLabels[j].address;
				break;
			}
		}
		if(foundAddress==0)
		{
			cout<<"Error: Invalid label"<<endl;
			ReportError();
		}
	}
	else if(OperationID==15)
	{
		RemoveSpaces(current_instruction);
		string tempString=findLabel();
		for(j=0;j<TableOfLabels.size();j++)
		{
			if(tempString==TableOfLabels[j].label)
			{
				r[0]=TableOfLabels[j].address;
			}
		}
	}
	else if(OperationID==16)
	{
		RemoveSpaces(current_instruction);
	}
	return OperationID;
}
void MIPSSimulator::OnlySpaces(int lower, int upper, string str)
{
	for(int i=lower;i<upper;i++)
	{
		if(str[i]!=' ' && str[i]!='\t')
		{
			cout<<"Error: Unexpected character"<<endl;
			ReportError();
		}
	}
}
void MIPSSimulator::ExecuteInstruction(int instruction)
{
	switch(instruction)
	{
		case 0:
			add();
			break;
		case 1:
			sub();
			break;
		case 2:
			mul();
			break;
		case 3:
			andf();
			break;
		case 4:
			orf();
			break;
		case 5:
			nor();
			break;
		case 6:
			slt();
			break;
		case 7:
			addi();
			break;
		case 8:
			andi();
			break;
		case 9:
			ori();
			break;
		case 10:
			slti();
			break;
		case 11:
			lw();
			break;
		case 12:
			sw();
			break;
		case 13:
			beq();
			break;
		case 14:
			bne();
			break;
		case 15:
			j();
			break;
		case 16:
			halt();
			break;
	}
}
void MIPSSimulator::RemoveSpaces(string &str)
{
	int j=0;
	int x=str[0];
	int y=' ';
	while(j<str.size() && (str[j]==' ' || str[j]=='\t'))
	{
		j++;
	}
	str=str.substr(j);
}
void MIPSSimulator::add()
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]+RegisterValues[r[2]]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1 && r[2]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]+RegisterValues[r[2]];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::addi()
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]+r[2]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]+r[2];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::sub()//Overflow check to be done?
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]-RegisterValues[r[2]]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1 && r[2]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]-RegisterValues[r[2]];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::mul()//last 32 bits?
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]*RegisterValues[r[2]]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1 && r[2]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]*RegisterValues[r[2]];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::andf()
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]&RegisterValues[r[2]]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1 && r[2]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]&RegisterValues[r[2]];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::andi()
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]&r[2]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]&r[2];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::orf()
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]|RegisterValues[r[2]]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1 && r[2]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]|RegisterValues[r[2]];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::ori()
{
	if(r[0]==29)
	{
		checkStackBounds(RegisterValues[r[1]]|r[2]);
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1)
	{
		RegisterValues[r[0]]=RegisterValues[r[1]]|r[2];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::nor()
{
	if(r[0]==29)
	{
		checkStackBounds(~(RegisterValues[r[1]]|RegisterValues[r[2]]));
	}
	if(r[0]!=0 && r[0]!=1 && r[1]!=1 && r[2]!=1)
	{
		RegisterValues[r[0]]=~(RegisterValues[r[1]]|RegisterValues[r[2]]);
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::slt()
{
	if(r[0]!=0 && r[0]!=1 && r[1]!=1 && r[2]!=1)
	{
		if(RegisterValues[r[1]]<RegisterValues[r[2]])
		{
			RegisterValues[r[0]]=1;
		}
		else
		{
			RegisterValues[r[0]]=0;
		}

	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::slti()
{
	if(r[0]!=0 && r[0]!=1 && r[1]!=1)
	{
		if(RegisterValues[r[1]]<r[2])
		{
			RegisterValues[r[0]]=1;
		}
		else
		{
			RegisterValues[r[0]]=0;
		}

	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::lw()
{
	if(r[0]==29)
	{
		checkStackBounds(r[1]);
	}
	if(r[0]!=0 && r[0]!=1 && r[2]==-1)
	{
		RegisterValues[r[0]]=r[1];
	}
	else if(r[0]!=0 && r[0]!=1)
	{
		checkStackBounds(RegisterValues[r[1]]+r[2]);
		RegisterValues[r[0]]=Stack[(RegisterValues[r[1]]+r[2]-40000)/4];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::sw()
{
	if(r[0]!=1 && r[2]==-1)
	{
		Memory[r[1]].value=RegisterValues[r[0]];
	}
	else if(r[0]!=1)
	{
		checkStackBounds(RegisterValues[r[1]]+r[2]);
		Stack[(RegisterValues[r[1]]+r[2]-40000)/4]=RegisterValues[r[0]];
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::beq()
{
	if(r[0]!=1 && r[1]!=1)
	{
		if(RegisterValues[r[0]]==RegisterValues[r[1]])
		{
			ProgramCounter=r[2];
		}
		else
		{
			ProgramCounter++;
		}
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::bne()
{
	if(r[0]!=1 && r[1]!=1)
	{
		if(RegisterValues[r[0]]!=RegisterValues[r[1]])
		{
			ProgramCounter=r[2];
		}
		else
		{
			ProgramCounter++;
		}
	}
	else
	{
		cout<<"Error: Invalid usage of registers"<<endl;
		ReportError();
	}
}
void MIPSSimulator::j()
{
	ProgramCounter=r[0];
}
void MIPSSimulator::halt()
{
	halt_value=1;
}
void MIPSSimulator::displayState()
{
	int current_address=40000;
	if(ProgramCounter<NumberOfInstructions)
	{
		cout<<endl<<"Executing instruction: "<<InputProgram[ProgramCounter]<<endl;
	}
	else
	{
		cout<<endl<<"Executing instruction: "<<InputProgram[ProgramCounter-1]<<endl;
	}
	cout<<endl<<"Program Counter: "<<(4*ProgramCounter)<<endl<<endl;
	cout<<"Registers:"<<endl<<endl;
	printf("%11s%12s\t\t%10s%12s\n","Register","Value","Register","Value");
	for(int i=0;i<16;i++)
	{
		printf("%6s[%2d]:%12d\t\t%5s[%2d]:%12d\n",Registers[i].c_str(),i,RegisterValues[i],Registers[i+16].c_str(),i+16,RegisterValues[i+16]);
	}
	cout<<endl<<"Memory:"<<endl<<endl;
	printf("%11s%11s%8s\n","Address","Label","Value");
	for(int i=0;i<100;i++)
	{
		printf("%11d%10s:%8d\n",current_address+4*i,"<Stack>",Stack[i]);
	}
	current_address+=400;
	for(int i=0;i<Memory.size();i++)
	{
		printf("%11d%10s:%8d\n",40400+4*i,Memory[i].label.c_str(),Memory[i].value);
	}
	cout<<endl;
}
void MIPSSimulator::assertNumber(string str)
{
	for(int j=0;j<str.size();j++)
	{
		if(str[j]<48 || str[j]>57)
		{
			cout<<"Error: Specified value is not a number"<<endl;
			ReportError();
		}
	}
	if(str[0]!='-' && (str.size()>10 || (str.size()==10 && str>"2147483647")))
	{
		cout<<"Error: Number out of range"<<endl;
		ReportError();
	}
	else if(str[0]=='-' && (str.size()>11 || (str.size()==11 && str>"-2147483648")))
	{
		cout<<"Error: Number out of range"<<endl;
		ReportError();
	}
}
void MIPSSimulator::findRegister(int number)
{
	int foundRegister=0;
	if(current_instruction[0]!='$' || current_instruction.size()<2)
	{
		cout<<"Error: Register expected"<<endl;
		ReportError();
	}
	current_instruction=current_instruction.substr(1);
	string registerID=current_instruction.substr(0,2);
	if(registerID=="ze" && current_instruction.size()>=4)
	{
		registerID+=current_instruction.substr(2,2);
	}
	else if(registerID=="ze")
	{
		cout<<"Error: Register expected"<<endl;
		ReportError();
	}
	for(int i=0;i<32;i++)
	{
		if(registerID==Registers[i])
		{
			r[number]=i;
			foundRegister=1;
			if(i!=0)
				current_instruction=current_instruction.substr(2);
			else
				current_instruction=current_instruction.substr(4);
		}
	}
	if(foundRegister==0)
	{
		cout<<"Error: Invalid register"<<endl;
		ReportError();
	}
}
string MIPSSimulator::findLabel()
{
	RemoveSpaces(current_instruction);
	string tempString="";
	int foundValue=0;
	int doneFinding=0;
	for(int j=0;j<current_instruction.size();j++)
	{
		if(foundValue==1 && (current_instruction[j]==' ' || current_instruction[j]=='\t') && doneFinding==0)
		{
			doneFinding=1;
		}
		else if(foundValue==1 && current_instruction[j]!=' ' && current_instruction[j]!='\t' && doneFinding==1)
		{
			cout<<"Error: Unexpected text after value"<<endl;
			ReportError();
		}
		else if(foundValue==0 && current_instruction[j]!=' ' && current_instruction[j]!='\t')
		{
			foundValue=1;
			tempString=tempString+current_instruction[j];
		}
		else if(foundValue==1 && current_instruction[j]!=' ' && current_instruction[j]!='\t')
		{
			tempString=tempString+current_instruction[j];
		}
	}
	return tempString;
}
void MIPSSimulator::assertRemoveComma()
{
	if(current_instruction.size()<2 || current_instruction[0]!=',')
	{
		cout<<"Error: Comma expected"<<endl;
		ReportError();
	}
	current_instruction=current_instruction.substr(1);
}
void MIPSSimulator::checkStackBounds(int index)
{
	if(!(index<=40396 && index>=40000 && index%4==0))
	{
		cout<<"Error: Invalid address for stack pointer. To access data section, use labels instead of addresses"<<endl;
		ReportError();
	}
}
int sortmemory(MemoryElement a, MemoryElement b)
{
	return a.label<b.label;
}
int sorttable(LabelTable a, LabelTable b)
{
	return a.label<b.label;
}
int main()
{
	string path;
	int mode;
	cout<<"Program to simulate execution in MIPS Assembly language. Two modes are available:"<<endl<<"1. Step by Step Mode"<<endl<<"2. Execution Mode"<<endl<<endl;
	cout<<"Enter the relative path of the input file and the mode:"<<endl;
	cin>>path>>mode;
	if(mode!=1 && mode!=2)
	{
		cout<<"Error: Invalid Mode"<<endl;
		return 1;
	}
	MIPSSimulator simulator(mode-1,path);
	simulator.Execute();
	return 0;
}
