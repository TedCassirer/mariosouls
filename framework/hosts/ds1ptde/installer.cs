// GPL-3.0-only. Small Windows front end; PTDE setup.py owns game modifications.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

class MarioSoulsSetup : Form {
    readonly TextBox game = new TextBox(), rom = new TextBox(), output = new TextBox();
    readonly Button install = new Button(), uninstall = new Button();
    readonly Label status = new Label();
    bool busy;
    readonly Dictionary<string, object> manifest;

    static byte[] Resource(string name) {
        using (var input = Assembly.GetExecutingAssembly().GetManifestResourceStream(name)) {
            if (input == null)
                throw new InvalidDataException("Missing installer resource: " + name);
            using (var bytes = new MemoryStream()) {
                input.CopyTo(bytes);
                return bytes.ToArray();
            }
        }
    }

    static string Hash(byte[] data) {
        using (var sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(data))
            .Replace("-", "")
            .ToLowerInvariant();
    }

    static Dictionary<string, object> ReadManifest() {
        return new JavaScriptSerializer().Deserialize<Dictionary<string, object>>(
            Encoding.UTF8.GetString(Resource("manifest.json")));
    }

    static void Extract(byte[] data, string directory, bool python = false) {
        Directory.CreateDirectory(directory);
        if ((File.GetAttributes(directory) & FileAttributes.ReparsePoint) != 0)
            throw new IOException("Installer cache must not be a link.");
        string root = Path.GetFullPath(directory) + Path.DirectorySeparatorChar;
        using (var archive = new ZipArchive(new MemoryStream(data), ZipArchiveMode.Read)) {
            long total = 0;
            foreach (var entry in archive.Entries) {
                total += entry.Length;
                if (total > 512L * 1024 * 1024 || entry.FullName.Contains(":") ||
                    entry.FullName.Contains("\\"))
                    throw new InvalidDataException("Invalid installer archive.");
                string path = Path.GetFullPath(Path.Combine(directory, entry.FullName));
                if (!path.StartsWith(root, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("Unsafe installer archive path.");
                for (var parent = new DirectoryInfo(Path.GetDirectoryName(path));
                     parent != null && parent.FullName.Length >= directory.Length;
                     parent = parent.Parent)
                    if (parent.Exists && (parent.Attributes & FileAttributes.ReparsePoint) != 0)
                        throw new IOException("Installer cache contains a link.");
                if (entry.FullName.EndsWith("/")) {
                    Directory.CreateDirectory(path);
                    continue;
                }
                if (File.Exists(path) &&
                    (File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0)
                    throw new IOException("Installer cache contains a link.");
                if (python && entry.FullName == "python313._pth")
                    continue;
                byte[] bytes;
                using (var src = entry.Open()) using (var dst = new MemoryStream()) {
                    src.CopyTo(dst);
                    bytes = dst.ToArray();
                }
                if (File.Exists(path) && Hash(File.ReadAllBytes(path)) == Hash(bytes))
                    continue;
                if (File.Exists(path))
                    throw new IOException("Installer cache changed: " + entry.FullName);
                Directory.CreateDirectory(Path.GetDirectoryName(path));
                using (var dst = new FileStream(path, FileMode.CreateNew, FileAccess.Write))
                    dst.Write(bytes, 0, bytes.Length);
            }
        }
    }

    static string Quote(string value) {
        // Windows CommandLineToArgvW quoting, including paths ending in backslashes.
        var result = new StringBuilder("\"");
        int slashes = 0;
        foreach (char c in value) {
            if (c == '\\') {
                slashes++;
                continue;
            }
            result.Append('\\', c == '"' ? slashes * 2 + 1 : slashes);
            result.Append(c);
            slashes = 0;
        }
        result.Append('\\', slashes * 2);
        return result.Append('"').ToString();
    }

    public MarioSoulsSetup() {
        manifest = ReadManifest();
        Text = "MarioSouls Setup " + manifest["version"];
        ClientSize = new Size(700, 530);
        StartPosition = FormStartPosition.CenterScreen;
        Font = new Font("Segoe UI", 9F);
        MinimumSize = MaximumSize = Size;
        MaximizeBox = false;
        var heading = new Label { Text = "MarioSouls for Dark Souls: Prepare to Die Edition",
                                  Left = 20,
                                  Top = 18,
                                  Width = 660,
                                  Height = 30,
                                  Font = new Font(Font, FontStyle.Bold) };
        var help = new Label {
            Text =
                "Experimental Windows PTDE mod. Remastered is not supported.\nFirst install downloads pinned tools and builds from source. Internet and free disk space are required.\nChoose your own US Super Mario 64 .z64 ROM. Game assets and ROMs are never downloaded.",
            Left = 20, Top = 54, Width = 660, Height = 70
        };
        Controls.Add(heading);
        Controls.Add(help);
        AddPath("Dark Souls folder", game, 135, delegate {
            using (var dialog = new FolderBrowserDialog()) { dialog.Description = "Select PTDE's DATA folder (contains DARKSOULS.exe)"; if (Directory.Exists(game.Text)) dialog.SelectedPath = game.Text; if (dialog.ShowDialog(this) == DialogResult.OK) game.Text = dialog.SelectedPath;
    }
});
        AddPath("SM64 ROM", rom, 199, delegate {
            using (var dialog = new OpenFileDialog()) { dialog.Filter = "US SM64 ROM (*.z64)|*.z64"; if (dialog.ShowDialog(this) == DialogResult.OK) rom.Text = dialog.FileName;
        }
        });
        game.Text =
            Environment.GetEnvironmentVariable("MARIOSOULS_GAME_DIR") ??
            @"C:\Program Files (x86)\Steam\steamapps\common\Dark Souls Prepare to Die Edition\DATA";
        install.Text = "Install / Update";
        install.SetBounds(20, 270, 160, 34);
        uninstall.Text = "Uninstall";
        uninstall.SetBounds(190, 270, 130, 34);
        install.Click += async delegate {
            await Start("install");
        };
        uninstall.Click += async delegate {
            await Start("uninstall");
        };
        Controls.Add(install);
        Controls.Add(uninstall);
        var source = new LinkLabel { Text = "Source, licenses and help", Left = 450, Top = 279,
                                     Width = 220 };
        source.LinkClicked += delegate {
            Process.Start(new ProcessStartInfo(
                "https://github.com/TedCassirer/mariosouls") { UseShellExecute = true });
        };
        Controls.Add(source);
        status.Text = "Close Dark Souls before installing or uninstalling.";
        status.SetBounds(20, 316, 660, 25);
        Controls.Add(status);
        output.Multiline = true;
        output.ReadOnly = true;
        output.ScrollBars = ScrollBars.Vertical;
        output.SetBounds(20, 346, 660, 164);
        Controls.Add(output);
        FormClosing += delegate(object sender, FormClosingEventArgs e) {
            if (busy) {
                e.Cancel = true;
                MessageBox.Show(this,
                                "Please wait for setup to finish. You can minimize this window.",
                                "Setup running");
            }
        };
        }

        void AddPath(string label, TextBox field, int top, Action browse) {
            Controls.Add(new Label { Text = label, Left = 20, Top = top, Width = 640 });
            field.SetBounds(20, top + 23, 570, 27);
            Controls.Add(field);
            var button = new Button { Text = "Browse...", Left = 600, Top = top + 22, Width = 80,
                                      Height = 28 };
            button.Click += delegate {
                if (!busy)
                    browse();
            };
            Controls.Add(button);
        }

        void Log(string line) {
            if (!String.IsNullOrEmpty(line))
                BeginInvoke((Action)delegate { output.AppendText(line + Environment.NewLine); });
        }

        async Task Start(string action) {
            if (busy)
                return;
            if (String.IsNullOrWhiteSpace(game.Text) ||
                (action == "install" && !File.Exists(rom.Text))) {
                MessageBox.Show(this,
                                "Choose the game folder and, for installation, your .z64 ROM.",
                                "Paths required");
                return;
            }
            string gamePath = game.Text, romPath = rom.Text;
            busy = true;
            install.Enabled = uninstall.Enabled = game.Enabled = rom.Enabled = false;
            output.Clear();
            status.Text = "Preparing setup...";
            try {
                int result = await Task.Run(() => Execute(action, gamePath, romPath));
                if (result != 0)
                    throw new InvalidOperationException(
                        "Setup stopped. See the log below; game replacements use the existing rollback safeguards.");
                status.Text =
                    action == "install"
                        ? "Installation complete. Launch Dark Souls normally through Steam."
                        : "Uninstalled. Your saves and owned assets were kept.";
            } catch (Exception error) {
                status.Text = "Setup stopped.";
                Log(error.Message);
                MessageBox.Show(
                    this,
                    error.Message +
                        "\nFor a protected game folder, close this tool and run it as administrator.",
                    "MarioSouls setup", MessageBoxButtons.OK, MessageBoxIcon.Error);
            } finally {
                busy = false;
                install.Enabled = uninstall.Enabled = game.Enabled = rom.Enabled = true;
            }
        }

        int Execute(string action, string gamePath, string romPath) {
            byte[] source = Resource("source.zip"), python = Resource("python.zip");
            if (Hash(source) != (string)manifest["source_sha256"] ||
                Hash(python) != (string)manifest["python_sha256"])
                throw new InvalidDataException("Installer payload hash mismatch.");
            string home = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "MarioSouls", "setup", ((string)manifest["commit"]).Substring(0, 12));
            string root = Path.Combine(home, "source"), runtime = Path.Combine(home, "python");
            Extract(source, root);
            Extract(python, runtime, true);
            // Embeddable Python is isolated: explicitly opt in only to this package's paths.
            string paths = "python313.zip\n.\nLib/site-packages\n" + Path.Combine(root, "tools") +
                           "\n" + Path.Combine(root, "work/python-deps") + "\n" +
                           Path.Combine(root, "framework/characters/mario64") + "\n" +
                           Path.Combine(root, "framework/hosts/ds1ptde") + "\nimport site\n";
            string interpreter = Path.Combine(runtime, "python.exe");
            File.WriteAllText(Path.Combine(runtime, "python313._pth"), paths,
                              new UTF8Encoding(false));
            var start = new ProcessStartInfo(interpreter) {
                Arguments = Quote(Path.Combine(root, "framework/hosts/ds1ptde/player_setup.py")) +
                            " " + action + " --game " + Quote(gamePath) +
                            (action == "install" ? " --rom " + Quote(romPath) : ""),
                WorkingDirectory = root,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8
            };
            start.EnvironmentVariables["PYTHONIOENCODING"] = "utf-8";
            start.EnvironmentVariables["PYTHONUNBUFFERED"] = "1";
            start.EnvironmentVariables["NO_COLOR"] = "1";
            using (var log = new StreamWriter(
                       Path.Combine(home, "setup.log"), false,
                       new UTF8Encoding(false))) using (var process =
                                                            new Process { StartInfo = start }) {
                object sync = new object();
                DataReceivedEventHandler receive =
                    delegate(object sender, DataReceivedEventArgs e) {
                    if (e.Data == null)
                        return;
                    lock (sync) {
                        log.WriteLine(e.Data);
                        log.Flush();
                    }
                    Log(e.Data);
                };
                process.OutputDataReceived += receive;
                process.ErrorDataReceived += receive;
                Log("Log: " + Path.Combine(home, "setup.log"));
                process.Start();
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();
                process.WaitForExit();
                return process.ExitCode;
            }
        }

        [STAThread]
        static int Main(string[] args) {
            try {
                if (args.Length == 2 && args[0] == "--inspect") {
                    var manifest = ReadManifest();
                    if (Hash(Resource("source.zip")) != (string)manifest["source_sha256"] ||
                        Hash(Resource("python.zip")) != (string)manifest["python_sha256"])
                        return 2;
                    File.WriteAllText(args[1], new JavaScriptSerializer().Serialize(manifest),
                                      new UTF8Encoding(false));
                    return 0;
                }
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new MarioSoulsSetup());
                return 0;
            } catch (Exception error) {
                MessageBox.Show(error.Message, "MarioSouls setup");
                return 1;
            }
        }
        }
