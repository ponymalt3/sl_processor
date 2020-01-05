#include "RTAsmTest.h"
#include <mtest.h>

class testNeuralNetLib : public mtest::test
{
};

MTEST(testNeuralNetLib,test_that_neural_net_evaluation_function_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(
  numNodes=4;
  numInputs=3;
  inputVec_local {1,2,3};
  weightMatrix { 1.7,4,5,6, -0.5,33,12,17, 0.23,1,1,1, -100,9,-20,7};
  array resultVec_local 4;
  array sumVec 4;

  evaluteNodes(numNodes,weightMatrix,inputVec_local,numInputs,resultVec_local,sumVec);
  )");
  
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
    
  tester.loadCode();
  tester.execute();
  
  tester.expectSymbolWithOffset("sumVec",0,1.7+4*1+5*2+6*3);
  tester.expectSymbolWithOffset("sumVec",1,-0.5+33*1+12*2+17*3);
  tester.expectSymbolWithOffset("sumVec",2,0.23+1*1+2*1+3*1);
  tester.expectSymbolWithOffset("sumVec",3,-100+9*1+-20*2+7*3);
  
  tester.expectSymbolWithOffset("resultVec_local",0,33.7);
  tester.expectSymbolWithOffset("resultVec_local",1,107.5);
  tester.expectSymbolWithOffset("resultVec_local",2,6.23);
  tester.expectSymbolWithOffset("resultVec_local",3,qfp32_t(0.01)*qfp32_t(-110.0));
}

MTEST(testNeuralNetLib,test_that_neural_net_dif_output_function_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(
  n=4;
  y_local {2,5,7,3};
  dif_out {2,5,7,3};
  output {-0.78,13.24,2.001,-7.64};

  difOutput(dif_out,output,sizeof(output));  

  a0=y_local;
  a1=output;
  delta=0.001;
  loop(4)
    t1=([a0]-([a1]-delta))*([a0]-([a1]-delta));
    t2=([a0]-([a1]+delta))*([a0]-([a1]+delta));
    [a0++]=(t2-t1)/0.002;
    [a1++]=[a1];
  end
  
  ## y_local == dif_out
  
  n=n; ## dummy)");
  
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();  
  tester.execute();
  
  tester.expectSymbolWithOffset("y_local",0,-5.55999279);
  tester.expectSymbolWithOffset("y_local",1,16.47970432);
  tester.expectSymbolWithOffset("y_local",2,-9.99800318);
  tester.expectSymbolWithOffset("y_local",3,-21.27865529);
  
  tester.expectSymbolWithOffset("dif_out",0,-5.55999994);
  tester.expectSymbolWithOffset("dif_out",1,16.48000001);
  tester.expectSymbolWithOffset("dif_out",2,-9.99800002);
  tester.expectSymbolWithOffset("dif_out",3,-21.27999997);
}

MTEST(testNeuralNetLib,test_that_neural_net_dif_layer_function_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(
  size=4;
  numInputs=3;
  vecInput_local {1,2,3};
  weightMatrix { 1.7,4,5,6, -0.5,33,12,17, 0.23,1,1,1, -100,9,-20,7};
  weightMatrixResult { 1.7,4,5,6, -0.5,33,12,17, 0.23,1,1,1, -100,9,-20,7};
  array nextDifVec 3;
  array vecSum 4;
  array result 4;

  array expect 4;
  expect2 {0.478,-0.834,0.029,0.0004}; # will be overwritten by difLayer

  evaluteNodes(size,weightMatrix,vecInput_local,numInputs,result,vecSum);  
  difOutput(expect2,result,sizeof(result));

  a0=expect;
  a1=expect2;
  loop(sizeof(expect2))
    [a0++]=[a1++];
  end

  ## function to test
  difLayer(expect2,vecSum,size,vecInput_local,numInputs,weightMatrixResult,nextDifVec,1);

  ## make weightMatrixResult to be only the change
  a0=weightMatrix;
  a1=weightMatrixResult;
  loop(sizeof(weightMatrix))
    [a1++]=[a1]-[a0++];
  end

  array result1 4;
  array result2 4;
  array wdifs 16;

  loop(sizeof(weightMatrix))
    a0=weightMatrix+i;
    t=[a0];
    [a0]=t-0.01;
    evaluteNodes(size,weightMatrix,vecInput_local,numInputs,result1,vecSum);
    a0=weightMatrix+i;
    [a0]=t+0.01;
    evaluteNodes(size,weightMatrix,vecInput_local,numInputs,result2,vecSum);
    a0=weightMatrix+i;
    [a0]=t;
    
    a0=result1+int(i/size);
    d=[a0];
    a0=result2+int(i/size);
    d=(d-[a0])/0.02;

    a0=expect+int(i/size);## expect array stores output dif
    d=d*[a0];
   
    a1=wdifs+i;
    [a1]=d;
  end

  idifs {0,0,0};
  loop(numInputs)
    a0=vecInput_local+i;
    t=[a0];
    [a0]=t+0.01;
            

    evaluteNodes(size,weightMatrix,vecInput_local,numInputs,result1,vecSum);
    a0=vecInput_local+i;
    [a0]=t-0.01;
    evaluteNodes(size,weightMatrix,vecInput_local,numInputs,result2,vecSum);
    a0=vecInput_local+i;
    [a0]=t;
    
    d=0;
    loop(size)
      a0=result1+j;
      t=[a0];
      a0=result2+j;
      t=(t-[a0])/0.02;
      a0=expect+j;## expect array stores output dif
      t=t*[a0];
      d=d+t;
    end
    
    a1=idifs+i;
    [a1]=[a1]+d;
  end

  ## wdifs == weightMatrixResult
  ## idifs == nextDifVec
  
  numInputs=3; ## dummy instr otherwise last loop will be executed only once)");
  
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);

  tester.loadCode();  
  tester.execute();
  
  tester.expectSymbolWithOffset("weightMatrixResult",0,-66.44400024);
  tester.expectSymbolWithOffset("weightMatrixResult",1,-66.44400024);
  tester.expectSymbolWithOffset("weightMatrixResult",2,-132.88800048);
  tester.expectSymbolWithOffset("weightMatrixResult",3,-199.33200073);
  tester.expectSymbolWithOffset("weightMatrixResult",4,-216.66799926);
  tester.expectSymbolWithOffset("weightMatrixResult",5,-216.66799926);
  tester.expectSymbolWithOffset("weightMatrixResult",6,-433.33599853);
  tester.expectSymbolWithOffset("weightMatrixResult",7,-650.00399780);
  tester.expectSymbolWithOffset("weightMatrixResult",8,-12.40200006);
  tester.expectSymbolWithOffset("weightMatrixResult",9,-12.40200006);
  tester.expectSymbolWithOffset("weightMatrixResult",10,-24.80400013);
  tester.expectSymbolWithOffset("weightMatrixResult",11,-37.20599365);
  tester.expectSymbolWithOffset("weightMatrixResult",12,0.02200317);
  tester.expectSymbolWithOffset("weightMatrixResult",13,0.02200794);
  tester.expectSymbolWithOffset("weightMatrixResult",14,0.04401588);
  tester.expectSymbolWithOffset("weightMatrixResult",15,0.06602382);
  
  tester.expectSymbolWithOffset("wdifs",0,-66.45825195);
  tester.expectSymbolWithOffset("wdifs",1,-66.45825195);
  tester.expectSymbolWithOffset("wdifs",2,-132.86581420);
  tester.expectSymbolWithOffset("wdifs",3,-199.32406616);
  tester.expectSymbolWithOffset("wdifs",4,-216.54916381);
  tester.expectSymbolWithOffset("wdifs",5,-216.54916381);
  tester.expectSymbolWithOffset("wdifs",6,-433.42897033);
  tester.expectSymbolWithOffset("wdifs",7,-650.14346313);
  tester.expectSymbolWithOffset("wdifs",8,-12.40200006);
  tester.expectSymbolWithOffset("wdifs",9,-12.40200006);
  tester.expectSymbolWithOffset("wdifs",10,-24.80400013);
  tester.expectSymbolWithOffset("wdifs",11,-37.20599365);
  tester.expectSymbolWithOffset("wdifs",12,0.02199190);
  tester.expectSymbolWithOffset("wdifs",13,0.02199190);
  tester.expectSymbolWithOffset("wdifs",14,0.04401010);
  tester.expectSymbolWithOffset("wdifs",15,0.06601518);
  
  tester.expectSymbolWithOffset("nextDifVec",0,7428.02389526);
  tester.expectSymbolWithOffset("nextDifVec",1,2945.07814025);
  tester.expectSymbolWithOffset("nextDifVec",2,4094.26792907);
  
  tester.expectSymbolWithOffset("idifs",0,7427.91120910);
  tester.expectSymbolWithOffset("idifs",1,2944.94471740);
  tester.expectSymbolWithOffset("idifs",2,4094.38119506);
}

MTEST(testNeuralNetLib,test_that_neural_net_create_function_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(
  
  nnet_config {3,4,2};

  mem1=createMem(256,256);
  mem2=createMem(512,512);

  nnet=nnetCreate(nnet_config,sizeof(nnet_config),mem1,mem2);

  ## layer struc
  ## size
  ## node outputs + sums
  ## weights (size * next layer size)

  ## nnet
  ## num layers
  ## layer struc * num layers
  ## dif buffer 1 (max(layer size))
  ## dif buffer 2 (max(layer size))
  ## num inputs
  ## num outputs
  )");
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();  
  tester.execute();
  
  qfp32_t nnetAddr=501;
  
  tester.expectSymbol("nnet",nnetAddr);
  
  tester.expectMemoryAt(nnetAddr+0,2.0);//num layers
  
  //2 * layer struc
  tester.expectMemoryAt(nnetAddr+1,4.0);
  tester.expectMemoryAt(nnetAddr+2,492.0);//exact memory location depend on allocator
  tester.expectMemoryAt(nnetAddr+3,1007.0);
  
  tester.expectMemoryAt(nnetAddr+4,2.0);
  tester.expectMemoryAt(nnetAddr+5,487.0);
  tester.expectMemoryAt(nnetAddr+6,995.0);
  
  tester.expectMemoryAt(nnetAddr+7,478);
  tester.expectMemoryAt(nnetAddr+8,482);
  
  tester.expectMemoryAt(nnetAddr+9,3.0);// num inputs
  tester.expectMemoryAt(nnetAddr+10,2.0);//num outputs
}

MTEST(testNeuralNetLib,test_that_neural_net_eval_function_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(
  
  nnet_config {3,2,2};

  mem1=createMem(256,256);
  mem2=createMem(512,512);
  
  nnet=nnetCreate(nnet_config,sizeof(nnet_config),mem1,mem2);
  
  input {1,2,3};
  outputs=nnetEval(nnet,input,sizeof(input));
  nnet=nnet;
  )");
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse(0,false,20).getNumErrors() == 0);
  
  std::cout<<"dis:\n"<<(tester.getDisAsmString())<<"\n";
  
  tester.loadCode();
  tester.getProcessor().executeUntilAddr(1163);
  
  //init weights
  
  qfp32_t weights1[8]=
  {
    -17.5,11,-99,2,
    0.5,-1.05,31.5,3
  };
  
  qfp32_t weightAddr1=1015.0;
  
  for(uint32_t i=0;i<sizeof(weights1)/4;++i)
  {
    tester.getProcessor().writeMemory(weightAddr1+(double)i,weights1[i]);
  }
  
  qfp32_t weights2[6]=
  {
    7,0.001,-2.5,
    -2,-1,9
  };
  
  qfp32_t weightAddr2=1007.0;
  
  for(uint32_t i=0;i<sizeof(weights2)/4;++i)
  {
    tester.getProcessor().writeMemory(weightAddr2+(double)i,weights2[i]);
  }

  tester.execute();
  
  qfp32_t nnetAddr=501;  
  tester.expectSymbol("nnet",nnetAddr);
  
  //weights layer 1
  tester.expectMemoryAt(nnetAddr+3,weightAddr1);
  
  //weights layer 2
  tester.expectMemoryAt(nnetAddr+6,weightAddr2);
  
  //calculate expected output
  qfp32_t inputs[4]={1.0,1.0,2.0,3.0};
  
  qfp32_t layerOutputs1[2];
  for(uint32_t i=0;i<2;++i)
  {
    layerOutputs1[i]=0;
    for(uint32_t j=0;j<4;++j)
    {
      layerOutputs1[i]=layerOutputs1[i]+weights1[i*4+j]*inputs[j];
    }
    
    if(layerOutputs1[i] < qfp32_t(0.0))
    {
      layerOutputs1[i]=layerOutputs1[i]*qfp32_t(0.01);
    }    
  }
  
  inputs[1]=layerOutputs1[0];
  inputs[2]=layerOutputs1[1];
  
  qfp32_t layerOutputs2[2];
  for(uint32_t i=0;i<2;++i)
  {
    layerOutputs2[i]=0;
    for(uint32_t j=0;j<3;++j)
    {
      layerOutputs2[i]=layerOutputs2[i]+weights2[i*3+j]*inputs[j];
    }
    
    if(layerOutputs2[i] < qfp32_t(0.0))
    {
      layerOutputs2[i]=layerOutputs2[i]*qfp32_t(0.01);
    }    
  }
  
  qfp32_t outputsAddr=491;  
  tester.expectSymbol("outputs",outputsAddr);
  
  tester.expectMemoryAt(outputsAddr+0,layerOutputs2[0]);
  tester.expectMemoryAt(outputsAddr+1,layerOutputs2[1]);
}

MTEST(testNeuralNetLib,test_that_neural_net_update_works_correct)
{
  std::string file=__FILE__;
  RTProg testCode=RTProg::createFromFile(file.substr(0,file.find_last_of("/")+1)+"nnet_resolved");
  
  testCode.append(R"(
  
  function error(nnet,inputs,numInputs,expects,numExpects,numIter)
    err=0;
    loop(numIter)
      outputs=nnetEval(nnet,inputs+i*numInputs,numInputs);
      a0=outputs;
      a1=expects+numExpects*i;
      loop(numExpects)
        t=[a0++]-[a1++];
        err=err+abs(t);
      end
    end
    return err;
  end
  
  nnet_config {1,8,4,2};

  mem1=createMem(256,256);
  mem2=createMem(512,512);
  
  nnet=nnetCreate(nnet_config,sizeof(nnet_config),mem1,mem2);
  
  input {0.1,0.2,0.3,  0.4,0.5,0.6,  0.7,0.8,0.9,  1.0,1.1,1.2,  1.3,1.4,1.5};

  expect
  {  
    0.30902,0.30902,
    0.58779,0.58779,
    0.80902,0.80902,
    0.95106,0.95106,
    1,1
  };
  
  err1=error(nnet,input,1,expect,2,5);
  
  loop(5)
    loop(5)
      outputs=nnetEval(nnet,input+j*1,1);
      nnetUpdate(nnet,input,outputs,expect+j*2);
    end
  end
  
  err2=error(nnet,input,1,expect,2,5);
  
  ok=0;
  
  if(err2 < err1)
    ok=1;
  end
  )");
  
  RTProgTester tester(testCode);
  EXPECT(tester.parse().getNumErrors() == 0);
  
  tester.loadCode();
  tester.execute();
    
  tester.expectSymbol("ok",1);
}