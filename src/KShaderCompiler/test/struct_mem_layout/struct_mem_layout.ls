// This is comments..

struct TestStructure
{
	float4 var0;  // float3
	int2 var1; // int2
};

int PFN_RW_Structure(TestStructure& arg, TestStructure% arg1, float3% arg3_array[]) 
{
	arg.var0 = float4(0.1, 0.2, 0.3, 0.4);
	arg.var1 = int2(5,6);
	
	arg.var0 = arg.var0 + arg1.var0;
	arg.var1 = arg.var1 + arg1.var1;
	
	arg3_array[3] = float3(2,3,4);
	float2 tmpArray[12];
	tmpArray[5] = float2(4,5);
	return 123;
}

void DotProductFloat8(float8% arg0, float8% arg1, float8% outArg) 
{
	bool myB = true;
	bool4 myB1 = float4(1,2,3,4) > float4(2,1,3,4);
	bool4 myB2 = float4(1,2,3,4) > float4(2,1,4,3);
	myB1 = myB1 && myB2;
	myB1.y = myB;
	outArg = arg0 * arg1 * 0.5 + 0.7;
}