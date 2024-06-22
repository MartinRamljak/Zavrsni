// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "Parse_WAV_To_Beatmap.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

extern "C" {
#include "../../../C++/Additional_Libraries/kiss_fft130/kiss_fft130/kiss_fft.c"
}

using namespace std;

FBeatmap UMyBlueprintFunctionLibrary::Parse_WAV_To_Beatmap(FString wav_file, double Instant_Rate, double Variance_Modifier, double C_Base)
{
    FBeatmap Beatmap;
    //ifstream wav;
    //ofstream beatmap;
    //string wav_file_name = TCHAR_TO_UTF8(*wav_file);
    //string txt_file_name;
    string next_chunk;
    int Subchunk1Size, SampleRate, Subchunk2Size, ByteRate;
    short int NumChannels, BitsPerSample, BlockAlign;
    Variance_Modifier = -Variance_Modifier / pow(10,26);//Variance_Modifier before division here default = -1.5714
    TArray< uint8 > wav_array;
    int wav_pointer=16;

    /*wav.open(wav_file_name, ios::binary);
    
    if (!wav)
    {
        cout << "Couldn't open file.";
        return Beatmap;
    }*/

    FFileHelper::LoadFileToArray(wav_array, *wav_file);

    Subchunk1Size = *(int*)&wav_array[wav_pointer];
    wav_pointer += 6;
    NumChannels = *(short int*)&wav_array[wav_pointer];
    wav_pointer += 2;
    SampleRate = *(int*)& wav_array[wav_pointer];
    wav_pointer += 4;
    ByteRate = *(int*)&wav_array[wav_pointer];
    wav_pointer += 4;
    BlockAlign = *(short int*)&wav_array[wav_pointer];
    wav_pointer += 2;
    BitsPerSample = *(short int*)&wav_array[wav_pointer];
    wav_pointer += 2;

    if (Subchunk1Size > 16)
    {
        wav_pointer += Subchunk1Size - 16;
    }

    next_chunk[0] = *(char*)&wav_array[wav_pointer];
    wav_pointer++;
    next_chunk[1] = *(char*)&wav_array[wav_pointer];
    wav_pointer++;
    next_chunk[2] = *(char*)&wav_array[wav_pointer];
    wav_pointer++;
    next_chunk[3] = *(char*)&wav_array[wav_pointer];
    wav_pointer++;
    while (strcmp(&next_chunk[0], "data"))
    {
        Subchunk2Size = *(int*)&wav_array[wav_pointer];

        wav_pointer += 4 + Subchunk2Size;

        next_chunk[0] = *(char*)&wav_array[wav_pointer];
        wav_pointer++;
        next_chunk[1] = *(char*)&wav_array[wav_pointer];
        wav_pointer++;
        next_chunk[2] = *(char*)&wav_array[wav_pointer];
        wav_pointer++;
        next_chunk[3] = *(char*)&wav_array[wav_pointer];
        wav_pointer++;
    }
    Subchunk2Size = *(int*)&wav_array[wav_pointer];
    wav_pointer += 4;

    /*  Parsing the headers ifstream
    
    wav.ignore(16);
    wav.read((char*)&Subchunk1Size, 4);
    wav.ignore(2);
    wav.read((char*)&NumChannels, 2);
    wav.read((char*)&SampleRate, 4);
    wav.read((char*)&ByteRate, 4);
    wav.read((char*)&BlockAlign, 2);
    wav.read((char*)&BitsPerSample, 2);

    if (Subchunk1Size > 16)
        wav.ignore(Subchunk1Size - 16);

    wav.read(&next_chunk[0], 4);
    while (strcmp(&next_chunk[0], "data"))
    {
        wav.read((char*)&Subchunk2Size, 4);
        wav.ignore(Subchunk2Size);
        wav.read(&next_chunk[0], 4);
    }

    wav.read((char*)&Subchunk2Size, 4);
    */

    /*  Simple beat detection

        // Beat detection
        short int left, right;
        int e_buffer[43] = {0}, e_buffer_pos = 0, song_start_pos = wav.tellg();

        beatmap.open("MyLove.txt", 'w');

        while (true)
        {
            int instant_e = 0;
            double variance = 0, avg_e = 0, c;

            // Reading next moment
            for (int i = 0; i < 1024; i++)
            {
                if (wav.peek() == EOF)
                    break;
                wav.read((char*)&left, 2);
                wav.read((char*)&right, 2);

                instant_e += left * left + right * right;
            }
            if (wav.peek() == EOF)
                break;

            //Average energy of the last 1s
            for (int i = 0; i < 43; i++)
            {
                avg_e += e_buffer[i];
            }
            avg_e = avg_e / 43;

            //Calculating lenience (c)
            for (int i = 0; i < 43; i++)
            {
                variance += pow(e_buffer[i]-avg_e,2);
            }
            variance = variance / 43;

            c = (-0.0025714*variance) + 1.5142857;

            //Updating the buffer
            e_buffer[e_buffer_pos] = instant_e;
            e_buffer_pos = (e_buffer_pos == 42) ? 0 : e_buffer_pos + 1;

            //Beat detection (this time for real)
            if (instant_e > c * avg_e && avg_e>500)
            {
                int song_current_pos = wav.tellg();

                beatmap << (double)(song_current_pos - song_start_pos)/SampleRate << ",";
            }
        }

    */

    // FFT Beat Detection
    int instant_size = SampleRate / Instant_Rate; //Instant_Rate default = 5
    int no_subbands = 4;
    kiss_fft_cfg config = kiss_fft_alloc(instant_size, 0, NULL, NULL);
    double e_buffer[4][5] = { -1 };
    int e_buffer_pos = 0, song_start_pos = wav_pointer;
    int* skip_in_subband = new int[no_subbands];
    for (int i = 0; i < no_subbands; i++)
    {
        skip_in_subband[i] = 0;
    }

    /*beatmap.open("MyLove.txt", 'w');

    if (!beatmap)
    {
        cout << "Couldn't open file.";
        return Beatmap;
    }*/

    while (true)
    {
        kiss_fft_cpx* in = new kiss_fft_cpx[instant_size], * out = new kiss_fft_cpx[instant_size];
        double* subband_inst_e = new double[no_subbands], * subband_avg_e = new double[no_subbands];
        double variance = 0, c;

        //  Reading next moment
        for (int i = 0; i < instant_size; i++)
        {
            short int left, right;
            /*
            if (wav.peek() == EOF)
                break;
            wav.read((char*)&left, 2);
            wav.read((char*)&right, 2);
            */
            if (wav_pointer + 2 >= wav_array.Num())
                break;
            left = *(short int*)&wav_array[wav_pointer];
            wav_pointer += 2;
            right = *(short int*)&wav_array[wav_pointer];
            wav_pointer += 2;
            in[i].r = left;
            in[i].i = right;
        }
        /*
        if (wav.peek() == EOF)
            break;
        */
        if (wav_pointer + 2 >= wav_array.Num())
            break;

        //  FFT
        kiss_fft(config, in, out);
        delete[] in;

        double* fft_buffer = new double[instant_size];
        for (int i = 0; i < instant_size; i++)
            fft_buffer[i] = pow(out[i].r, 2) + pow(out[i].i, 2);
        delete[] out;

        //  Splitting into subbands
        for (int i = 0; i < no_subbands; i++)
        {
            subband_inst_e[i] = 0;
            for (int j = i * instant_size / no_subbands; j < (i + 1) * instant_size / no_subbands; j++) //devlog: no_subbands, instant_size ????
            {
                    subband_inst_e[i] += fft_buffer[j];
            }
            subband_inst_e[i] /= (double)5;
        }

        delete[] fft_buffer;

        // Calculating average energy per subband
        for (int i = 0; i < no_subbands; i++)
        {
            double k = 0;
            subband_avg_e[i] = 0;
            for (int j = 0; j < 5; j++)
            {
                if (e_buffer[i][j] != -1)
                {
                    subband_avg_e[i] += e_buffer[i][j];
                    k++;
                }
            }
            if (k == 0)
                subband_avg_e[i] = 0;
            else
                subband_avg_e[i] /= k;
        }

        //  Actual beat detection
        wstring Notes_To_Spawn = L"";
        int song_current_pos = wav_pointer;

        for (int i = 0; i < no_subbands; i++)
        {
            double k = 0;
            for (int j = 0; j < 5; j++)
            {
                if (e_buffer[i][j] != 0)
                {
                    variance += pow(e_buffer[i][j] - subband_avg_e[i], 2);
                    k++;
                }
            }
            if (k < 1)
                variance = 0;
            else
                variance /= k - 1;

            c = (Variance_Modifier * variance) + C_Base; //Variance_Modifier default = -0.0025714, C_Base default = 1.5142857
            if (c < 1)
                c = C_Base;

            if (skip_in_subband[i])
                skip_in_subband[i]--;
            else if ((subband_inst_e[i] > c * subband_avg_e[i]) && (subband_inst_e[i] > 100000000000))
            {
                Notes_To_Spawn += to_wstring(i);
                skip_in_subband[i] = Instant_Rate / 5;
                //beatmap << i << ":" << (double)(song_current_pos - song_start_pos) / (2 * NumChannels * SampleRate) << ",inst_e:" << subband_inst_e[i] << ",avg_e:" << subband_avg_e[i] << ",c:" << c << endl;
            }
        }
        if (!Notes_To_Spawn.empty())
        {
            Beatmap.Time_Since_Beginning_Arr.Add((double)(song_current_pos - song_start_pos) / (2 * NumChannels * SampleRate));
            Beatmap.Notes_To_Spawn_Arr.Add(FString(Notes_To_Spawn.c_str()));
        }

        //  Updating buffer
        for (int i = 0; i < no_subbands; i++)
            e_buffer[i][e_buffer_pos] = subband_inst_e[i];
        e_buffer_pos = (e_buffer_pos == 4) ? 0 : e_buffer_pos + 1;
    }

    free(config);

    //wav.close();

    //beatmap.close();

    //cout << Subchunk1Size << endl << NumChannels << endl << SampleRate << endl << BitsPerSample << endl << Subchunk2Size;

    return Beatmap;
}
