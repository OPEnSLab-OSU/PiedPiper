classdef GenerateNoisyAudio_exported < matlab.apps.AppBase

    % Properties that correspond to app components
    properties (Access = public)
        UIFigure             matlab.ui.Figure
        UploadSignalButton   matlab.ui.control.Button
        UploadNoiseButton    matlab.ui.control.Button
        GaussianNoiseButton  matlab.ui.control.Button
        SNRSliderLabel       matlab.ui.control.Label
        SNRSlider            matlab.ui.control.Slider
        snrLabel             matlab.ui.control.Label
        EditField            matlab.ui.control.NumericEditField
        AveragePowerLabel    matlab.ui.control.Label
        signalPower          matlab.ui.control.Label
        AveragePowerLabel_2  matlab.ui.control.Label
        signalPower_2        matlab.ui.control.Label
        AveragePowerLabel_3  matlab.ui.control.Label
        signalPower_3        matlab.ui.control.Label
        SaveSignalButton     matlab.ui.control.Button
        SignalAxis           matlab.ui.control.UIAxes
        NoiseAxis            matlab.ui.control.UIAxes
        NoisySignalAxis      matlab.ui.control.UIAxes
        Signal_spec          matlab.ui.control.UIAxes
        Noise_spec           matlab.ui.control.UIAxes
        Noisy_signal_spec    matlab.ui.control.UIAxes
    end

    
    properties (Access = private)
        signal %signal audio
        noise % noisy audio
        Fs %signal sample rate
        nFs %Noise Sample rate
        SNR = 0;%signal to noise ratio
        signal_file
        noise_file
        noisy_signal %additive noise onto signal
        noise_power
        signal_power
        noisy_signal_power
        
        noise_power_db
        signal_power_db
        noisy_signal_power_db
    end
    
    methods (Access = private)
        
        
      
        
        function [avg,avgDB] = averagePower(app,s)
                sum=0;
                for i=1:length(s)
                    sum=sum+s(i).^2;
                end
                avg = sum/length(s);
                avgDB = db(sum/length(s));
                
        end
    end
    
    methods (Access = public)
        
    end
    

    % Callbacks that handle component events
    methods (Access = private)

        % Button pushed function: UploadSignalButton
        function UploadSignalButtonPushed(app, event)
            app.signal_file = uigetfile('.wav');
            if ~isequal(app.signal_file,0)
                [app.signal,app.Fs] = audioread(app.signal_file); 
                plot(app.SignalAxis,app.signal);
              
                [s,f,t] = spectrogram(app.signal,[],[],[],app.Fs,'yaxis');
                imagesc(app.Signal_spec,t,f,log(1+abs(s)));
                set(app.Signal_spec,'YDir','normal');
                axis(app.Signal_spec,[0 numel(app.signal)*1/app.Fs 0 1000])
                
               
                [app.signal_power,app.signal_power_db] = app.averagePower(app.signal);
                text = sprintf('%.3f',app.signal_power_db);
                text = strcat(text,' dB');
                app.signalPower.Text = text;
            end
        end

        % Button pushed function: UploadNoiseButton
        function UploadNoiseButtonPushed(app, event)
            if numel(app.signal) == 0
                fig = uifigure;
                uialert(fig,'Upload a file to Signal first','No file in Signal','CloseFcn',@(h,e) close(fig));
            else
                app.noise_file = uigetfile('.wav');
                if ~isequal(app.noise_file,0)
                    [app.noise,app.nFs] = audioread(app.noise_file); 
                    app.noise = app.noise(1:numel(app.signal));
                    plot(app.NoiseAxis,app.noise);
                   
                   
                    [s,f,t] = spectrogram(app.noise,[],[],[],app.Fs,'yaxis');   %plot noise spectrogram
                    imagesc(app.Noise_spec,t,f,log(1+abs(s)));
                    set(app.Noise_spec,'YDir','normal');
                    axis(app.Noise_spec,[0 numel(app.signal)*1/app.Fs 0 1000])
                     
                   
                    app.noisy_signal = app.signal+app.noise;    %generate noisy signal
                    plot(app.NoisySignalAxis,app.noisy_signal); %plot noisy signal
                    [app.noise_power,app.noise_power_db] = app.averagePower(app.noise); %calculate signal power
                    text = sprintf('%.3f',app.noise_power_db);
                    text = strcat(text,' dB');
                    app.signalPower_2.Text = text;
                    
                    [s,f,t] = spectrogram(app.noisy_signal,[],[],[],app.Fs,'yaxis');    %plot noisy signal spectrogram
                    imagesc(app.Noisy_signal_spec,t,f,log(1+abs(s)));
                    set(app.Noisy_signal_spec,'YDir','normal');
                    axis(app.Noisy_signal_spec,[0 numel(app.signal)*1/app.Fs 0 1000])
                    
                    [app.noisy_signal_power,app.noisy_signal_power_db] = app.averagePower(app.noisy_signal);    %calculate noisy signal power
                    text = sprintf('%.3f',app.noisy_signal_power_db);
                    text = strcat(text,' dB');
                    app.signalPower_3.Text = text;
                end
            end
          
        end

        % Value changed function: SNRSlider
        function SNRSliderValueChanged(app, event)
            value = app.SNRSlider.Value;
            value = abs(round(value));
            app.SNR = value;
            text = sprintf('%d',value);
            app.snrLabel.Text = text;
            if numel(app.noisy_signal)~=0
                ratio = app.signal_power/app.noise_power; %calculate the ratio between signal and noise power
                t_factor = app.SNR/ratio;   %determine the factor that signal is greater than noise
                n = app.noise./sqrt(t_factor);  %divide the noise by the average power factor
                plot(app.NoiseAxis,n);  %plot the weakened noise
                
                text=sprintf('%d',app.noise_power/t_factor);
                app.signalPower_2.Text = text;
                
                app.noisy_signal = app.signal+n;
                plot(app.NoisySignalAxis,app.noisy_signal); %plot noisy signal
                
                [app.noise_power,app.noise_power_db] = app.averagePower(app.noise); %calculate signal power
                text = sprintf('%.3f',app.noise_power_db);
                text = strcat(text,' dB');
                app.signalPower_2.Text = text;
                    
                [s,f,t] = spectrogram(app.noisy_signal,[],[],[],app.Fs,'yaxis');    %plot noisy signal spectrogram
                imagesc(app.Noisy_signal_spec,t,f,log(1+abs(s)));
                set(app.Noisy_signal_spec,'YDir','normal');
                axis(app.Noisy_signal_spec,[0 numel(app.signal)*1/app.Fs 0 1000])
            end
            
            
        end

        % Value changed function: EditField
        function EditFieldValueChanged(app, event)
            value = app.EditField.Value;
            value = abs(round(value));
            app.SNR = value;
            text = sprintf('%d',value);
            app.snrLabel.Text = text;
            if numel(app.noisy_signal)~=0
                ratio = app.signal_power/app.noise_power; %calculate the ratio between signal and noise power
                t_factor = app.SNR/ratio;   %determine the factor that signal is greater than noise
                n = app.noise./sqrt(t_factor);  %divide the noise by the average power factor
                plot(app.NoiseAxis,n);  %plot the weakened noise
                
                text=sprintf('%d',app.noise_power/t_factor);
                app.signalPower_2.Text = text;
                
                app.noisy_signal = app.signal+n;
                plot(app.NoisySignalAxis,app.noisy_signal); %plot noisy signal
                
                [app.noise_power,app.noise_power_db] = app.averagePower(app.noise); %calculate signal power
                text = sprintf('%.3f',app.noise_power_db);
                text = strcat(text,' dB');
                app.signalPower_2.Text = text;
                    
                [s,f,t] = spectrogram(app.noisy_signal,[],[],[],app.Fs,'yaxis');    %plot noisy signal spectrogram
                imagesc(app.Noisy_signal_spec,t,f,log(1+abs(s)));
                set(app.Noisy_signal_spec,'YDir','normal');
                axis(app.Noisy_signal_spec,[0 numel(app.signal)*1/app.Fs 0 1000])
            end
        end

        % Button pushed function: GaussianNoiseButton
        function GaussianNoiseButtonPushed(app, event)
            if numel(app.signal) ~= 0
             app.noise = wgn(numel(app.signal),1,app.SNR);
             plot(app.NoiseAxis,app.noise);
             app.noisy_signal = awgn(app.signal,app.SNR);
             plot(app.NoisySignalAxis,app.noisy_signal);
             
             [app.noisy_signal_power,app.noisy_signal_power_db] = app.averagePower(app.noisy_signal);
             text = sprintf('%.3f',app.noisy_signal_power_db);
             text = strcat(text,' dB');
             app.signalPower_3.Text = text;
             
             [s,f,t] = spectrogram(app.noise,[],[],[],app.Fs,'yaxis');   %plot noise spectrogram
             imagesc(app.Noise_spec,t,f,log(1+abs(s)));
             set(app.Noise_spec,'YDir','normal');
             axis(app.Noise_spec,[0 numel(app.signal)*1/app.Fs 0 1000])
             colorbar(app.Noise_spec);
             
             [s,f,t] = spectrogram(app.noisy_signal,[],[],[],app.Fs,'yaxis');    %plot noisy signal spectrogram
             imagesc(app.Noisy_signal_spec,t,f,log(1+abs(s)));
             set(app.Noisy_signal_spec,'YDir','normal');
             axis(app.Noisy_signal_spec,[0 numel(app.signal)*1/app.Fs 0 1000])
             
            end
        end

        % Button pushed function: SaveSignalButton
        function SaveSignalButtonPushed(app, event)
            if (numel(app.noisy_signal) == 0)
                fig = uifigure;
                uialert(fig,'Upload a file to Signal and Noise first','No file in Signal and Noise','CloseFcn',@(h,e) close(fig));
            else
                file = uiputfile(strcat('Noise_Added_',app.signal_file));
                if ~isequal(file,0)
                    audiowrite(file,app.noisy_signal,app.Fs);
                end
            end
        end
    end

    % Component initialization
    methods (Access = private)

        % Create UIFigure and components
        function createComponents(app)

            % Create UIFigure and hide until all components are created
            app.UIFigure = uifigure('Visible', 'off');
            app.UIFigure.Position = [100 100 1036 664];
            app.UIFigure.Name = 'MATLAB App';

            % Create UploadSignalButton
            app.UploadSignalButton = uibutton(app.UIFigure, 'push');
            app.UploadSignalButton.ButtonPushedFcn = createCallbackFcn(app, @UploadSignalButtonPushed, true);
            app.UploadSignalButton.Position = [11 84 100 22];
            app.UploadSignalButton.Text = 'Upload Signal';

            % Create UploadNoiseButton
            app.UploadNoiseButton = uibutton(app.UIFigure, 'push');
            app.UploadNoiseButton.ButtonPushedFcn = createCallbackFcn(app, @UploadNoiseButtonPushed, true);
            app.UploadNoiseButton.Position = [11 50 100 22];
            app.UploadNoiseButton.Text = 'Upload Noise';

            % Create GaussianNoiseButton
            app.GaussianNoiseButton = uibutton(app.UIFigure, 'push');
            app.GaussianNoiseButton.ButtonPushedFcn = createCallbackFcn(app, @GaussianNoiseButtonPushed, true);
            app.GaussianNoiseButton.Position = [11 16 100 22];
            app.GaussianNoiseButton.Text = 'Gaussian Noise';

            % Create SNRSliderLabel
            app.SNRSliderLabel = uilabel(app.UIFigure);
            app.SNRSliderLabel.HorizontalAlignment = 'right';
            app.SNRSliderLabel.Position = [135 37 31 22];
            app.SNRSliderLabel.Text = 'SNR';

            % Create SNRSlider
            app.SNRSlider = uislider(app.UIFigure);
            app.SNRSlider.Limits = [1 100];
            app.SNRSlider.ValueChangedFcn = createCallbackFcn(app, @SNRSliderValueChanged, true);
            app.SNRSlider.Position = [187 46 84 3];
            app.SNRSlider.Value = 1;

            % Create snrLabel
            app.snrLabel = uilabel(app.UIFigure);
            app.snrLabel.HorizontalAlignment = 'center';
            app.snrLabel.Position = [138 16 28 22];
            app.snrLabel.Text = '0';

            % Create EditField
            app.EditField = uieditfield(app.UIFigure, 'numeric');
            app.EditField.Limits = [0.001 100];
            app.EditField.ValueChangedFcn = createCallbackFcn(app, @EditFieldValueChanged, true);
            app.EditField.Position = [182 58 24 22];
            app.EditField.Value = 1;

            % Create AveragePowerLabel
            app.AveragePowerLabel = uilabel(app.UIFigure);
            app.AveragePowerLabel.Position = [287 583 94 22];
            app.AveragePowerLabel.Text = 'Average Power: ';

            % Create signalPower
            app.signalPower = uilabel(app.UIFigure);
            app.signalPower.Position = [291 562 81 22];
            app.signalPower.Text = '0';

            % Create AveragePowerLabel_2
            app.AveragePowerLabel_2 = uilabel(app.UIFigure);
            app.AveragePowerLabel_2.Position = [287 409 94 22];
            app.AveragePowerLabel_2.Text = 'Average Power: ';

            % Create signalPower_2
            app.signalPower_2 = uilabel(app.UIFigure);
            app.signalPower_2.Position = [287 388 81 22];
            app.signalPower_2.Text = '0';

            % Create AveragePowerLabel_3
            app.AveragePowerLabel_3 = uilabel(app.UIFigure);
            app.AveragePowerLabel_3.Position = [287 230 94 22];
            app.AveragePowerLabel_3.Text = 'Average Power: ';

            % Create signalPower_3
            app.signalPower_3 = uilabel(app.UIFigure);
            app.signalPower_3.Position = [291 209 81 22];
            app.signalPower_3.Text = '0';

            % Create SaveSignalButton
            app.SaveSignalButton = uibutton(app.UIFigure, 'push');
            app.SaveSignalButton.ButtonPushedFcn = createCallbackFcn(app, @SaveSignalButtonPushed, true);
            app.SaveSignalButton.Position = [664 16 100 22];
            app.SaveSignalButton.Text = 'Save Signal';

            % Create SignalAxis
            app.SignalAxis = uiaxes(app.UIFigure);
            title(app.SignalAxis, 'Signal')
            xlabel(app.SignalAxis, 'Time(s)')
            ylabel(app.SignalAxis, 'Amplitude')
            zlabel(app.SignalAxis, 'Z')
            app.SignalAxis.Position = [11 500 275 157];

            % Create NoiseAxis
            app.NoiseAxis = uiaxes(app.UIFigure);
            title(app.NoiseAxis, 'Noise')
            xlabel(app.NoiseAxis, 'Time(s)')
            ylabel(app.NoiseAxis, 'Amplitude')
            zlabel(app.NoiseAxis, 'Z')
            app.NoiseAxis.Position = [11 330 275 157];

            % Create NoisySignalAxis
            app.NoisySignalAxis = uiaxes(app.UIFigure);
            title(app.NoisySignalAxis, 'Noisy Signal')
            xlabel(app.NoisySignalAxis, 'Time(s)')
            ylabel(app.NoisySignalAxis, 'Amplitude')
            zlabel(app.NoisySignalAxis, 'Z')
            app.NoisySignalAxis.Position = [13 153 275 157];

            % Create Signal_spec
            app.Signal_spec = uiaxes(app.UIFigure);
            title(app.Signal_spec, 'Signal Spectrogram')
            xlabel(app.Signal_spec, 'Time(s)')
            ylabel(app.Signal_spec, 'Frequency(Hz)')
            zlabel(app.Signal_spec, 'Z')
            app.Signal_spec.Position = [448 409 258 238];

            % Create Noise_spec
            app.Noise_spec = uiaxes(app.UIFigure);
            title(app.Noise_spec, 'Noise Spectrogram')
            xlabel(app.Noise_spec, 'Time(s)')
            ylabel(app.Noise_spec, 'Frequency(Hz)')
            zlabel(app.Noise_spec, 'Z')
            app.Noise_spec.Position = [724 409 256 238];

            % Create Noisy_signal_spec
            app.Noisy_signal_spec = uiaxes(app.UIFigure);
            title(app.Noisy_signal_spec, 'Noisy Signal Spectrogram')
            xlabel(app.Noisy_signal_spec, 'Time(s)')
            ylabel(app.Noisy_signal_spec, 'Frequency(Hz)')
            zlabel(app.Noisy_signal_spec, 'Z')
            app.Noisy_signal_spec.PlotBoxAspectRatio = [1.89453125 1 1];
            app.Noisy_signal_spec.Position = [448 84 532 310];

            % Show the figure after all components are created
            app.UIFigure.Visible = 'on';
        end
    end

    % App creation and deletion
    methods (Access = public)

        % Construct app
        function app = GenerateNoisyAudio_exported

            % Create UIFigure and components
            createComponents(app)

            % Register the app with App Designer
            registerApp(app, app.UIFigure)

            if nargout == 0
                clear app
            end
        end

        % Code that executes before app deletion
        function delete(app)

            % Delete UIFigure when app is deleted
            delete(app.UIFigure)
        end
    end
end