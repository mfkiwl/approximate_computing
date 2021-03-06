#include "exp_poly_appr.hpp"

void EXP_POLY_APPR::Init(vector<double> param1, vector<double> param2, uint64_t param3, double param4, string param5)
{
    coeff = param1;
    order = param2;
    fracWidth = param3;
    threshold = param4;
    fixed_sim = param5;
    input = 0;
    output = 0;
    term_cnt = coeff.size();
    mae = 0;
    mse = 0;
    mre = 0;
    wce = 0;
    wcre = 0;
    poly_appr.Init(coeff, order, fracWidth, fixed_sim);

    // exp(1) is around 2.7, requiring 2 bits integer, so have format of {2-bit.(fracWidth-2)-bit}
    truncation.Init(fracWidth-2);
    exp1 = truncation.Out(exp(1));

    totalCnt = 1UL<<fracWidth;
    // printf("%d\n", totalCnt);
    N = 64;

    weight.resize(totalCnt);
    for (int i = 0; i < totalCnt; ++i)
    {
        weight[i] = 0;
    }
    
    // // Gaussian distribution
    // double number;
    // default_random_engine generator;
    // normal_distribution<double> distribution(0.25,0.1);
    // for (int i = 0; i < (totalCnt)*N; ++i)
    // {
    //     number = distribution(generator);
    //     if ((number>=0.0) && (number<1.0))
    //     {
    //         ++weight[int(number*totalCnt)];
    //     }
    // }

    // uniform distribution
    for (int i = 0; i < (totalCnt); ++i)
    {
        weight[i] = N;
    }

    // step distribution
    // for (int i = 0; i < (totalCnt); ++i)
    // {
    //     if (i < totalCnt/2)
    //     {
    //         weight[i] = 0;
    //     }
    //     else
    //     {
    //         weight[i] = N*2;
    //     }
    // }

    // for (int i = 0; i < (totalCnt); ++i)
    // {
    //     printf("%d\n", weight[i]);
    //     for (int j = 0; j < weight[i]; ++j)
    //     {
    //         printf("*");
    //     }
    //     printf("\n");
    // }

}

void EXP_POLY_APPR::Report()
{
    printf("Current EXP_POLY_APPR:\n");
    std::cout << "Instance Mode:          " << fixed_sim << std::endl;
    printf("Number of Term:         %-5llu\n", term_cnt);
    printf("Operand Fraction Width: %-5llu\n", fracWidth);
}

void EXP_POLY_APPR::Help()
{
    printf("**********************************************************\n");
    printf("**********************************************************\n");
    printf("\nCalling EXP_POLY_APPR Help. Following are instructions to EXP_POLY_APPR Instance Usage:\n");
    printf("\n1. inst.Init() method:\n");
    printf("Configure the EXP_POLY_APPR inst.\n");
    printf("Parameters: Vector of Coefficients, Vector of Orders, Data Fraction Width, Operating Mode\n");
    printf("Input is fracWidth-bit fraction, output is {2-bit.fracWidth-bit}.\n");

    printf("\n2. inst.Report() method:\n");
    printf("Report Instance Configuration.\n");

    printf("\n3. inst.Out() method:\n");
    printf("Return the output for given input to method Inst.Out().\n");

    printf("\n4. inst.Eval() method:\n");
    printf("Return the evaluation result for cerntain input distribution.\n");

    printf("\n5. inst.MAE() method:\n");
    printf("Return the Mean Absolute Error (MAE) for cerntain input distribution.\n");

    printf("\n6. inst.MSE() method:\n");
    printf("Return the Mean Square Error (MSE) for cerntain input distribution.\n");

    printf("\n7. inst.MRE() method:\n");
    printf("Return the Mean Relative Error (MRE) for cerntain input distribution.\n");
    
    printf("\n8. inst.WCE() method:\n");
    printf("Return the Worst Case Error (WCE) for cerntain input distribution.\n");
    
    printf("\n9. inst.WCRE() method:\n");
    printf("Return the Worst Case Relative Error (WCRE) for cerntain input distribution.\n\n");
    printf("**********************************************************\n");
    printf("**********************************************************\n");
}

double EXP_POLY_APPR::Out(double param1)
{
    input = param1;
    // output = poly_appr.Out(input);

    // output at this point has format {2.fracWidth}
    if (input < threshold)
    {
        // printf("less\n");
        output = poly_appr.Out(input);
    }
    else
    {
        // printf("more\n");
        // printf("%lf\n", poly_appr.Out(input-1));
        output = poly_appr.Out(input-1);
    }

    if (input < threshold)
    {
        // {2,fracWidth-2} output multiply by e^0 with format {2,fracWidth-2}
        output = mul_fixed.Out(truncation.Out(output), 1, fracWidth-2);
    }
    else
    {
        // {2,fracWidth-2} output multiply by e^1 with format {2,fracWidth-2}
        output = mul_fixed.Out(truncation.Out(output), exp1, fracWidth-2);
    }
    // output at this point has format {2.fracWidth-2}
    return output;
}

void EXP_POLY_APPR::Eval()
{
    mae = 0;
    mse = 0;
    mre = 0;
    wce = 0;
    wcre = 0;
    double appr;
    double orig;
    double in;
    double absError;

    for (int i = 0; i < (totalCnt); ++i)
    {
        in = (double)(i/pow(2,fracWidth));
        orig = exp(in);
        // appr at this point has format {2.fracWidth}
        if (in < threshold)
        {
            // printf("less\n");
            appr = poly_appr.Out(in);
        }
        else
        {
            // printf("more\n");
            // printf("%lf\n", poly_appr.Out(in-1));
            appr = poly_appr.Out(in-1);
        }

        if (in < threshold)
        {
            // {2,fracWidth-2} appr multiply by e^0 with format {2,fracWidth-2}
            appr = truncation.Out(appr);
        }
        else
        {
            // {2,fracWidth-2} appr multiply by e^1 with format {2,fracWidth-2}
            appr = mul_fixed.Out(truncation.Out(appr), exp1, fracWidth-2);
        }

        absError = abs(appr - orig);
        mae += absError * weight[i];
        mse += pow(absError,2.0) * weight[i];
        mre += absError/orig * weight[i];
        if (wce < absError)
        {
            wce = absError;
        }
        if (wcre < absError/orig)
        {
            wcre = absError/orig;
        }
    }
    mae /= (pow(2,fracWidth)*N);
    mse /= (pow(2,fracWidth)*N);
    mre /= (pow(2,fracWidth)*N);
}

double EXP_POLY_APPR::MAE()
{
    return mae;
}

double EXP_POLY_APPR::MSE()
{
    return mse;
}

double EXP_POLY_APPR::MRE()
{
    return mre;
}

double EXP_POLY_APPR::WCE()
{
    return wce;
}

double EXP_POLY_APPR::WCRE()
{
    return wcre;
}
