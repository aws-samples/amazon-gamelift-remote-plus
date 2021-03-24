# amazon-gamelift-remote-plus

## Introduction

Users of Amazon GameLift have from time to time requested help accessing fleet instances using Secure Shell (SSH) for fleets running Linux, and Remote Desktop Protocol (RDP) for fleets running Windows. Performing these operations manually each time is laborious, so this tool turns these remote connections into a couple of clicks. More details are given about the features in the Features section below.

![screenshot](doc/img/app.png)

You need this project if:
* You use Amazon GameLift
* You need to connect to fleet instances using SSH or RDP
* You have (or can obtain) credentials to call AWS SDK commands on your fleets
* You are running Windows locally

## Building the project

### Prerequisites

You must have Visual Studio 2017 or 2019 installed to build this project from source. Either Community, Professional or Enterprise editions will work. You must have the following features installed:

* .NET Framework 4.5 targeting pack
* C# Roslyn compiler
* C++/CLI support
* MSBuild
* Platform toolset v141 (or you could open the solution in Visual Studio to upgrade the project files to a later toolset that you have installed.)
* C# and Visual Basic
* Visual Studio C++ Core Features

These features are all installed if you selected .NET desktop development, and Desktop development with C++ workloads when you installed Visual Studio. Features can be selected in the Visual Studio Installer.

### Getting the files

You must have git installed to clone the repository.

To clone the project repository, issue this command at the command line. Ensure that you are working in the directory that wish to clone the project into. For example if you are in directory ```C:\dev\``` then the project will end up in the directory ```C:\dev\amazon-gamelift-remote-plus\``` and the build directory will then be ```C:\dev\amazon-gamelift-remote-plus\build\```

```bat
git clone https://github.com/awslabs/amazon-gamelift-remote-plus.git
```

### Compiling the binaries

To build the project, issue these commands at the command line.

```bat
cd build
build
run
```

The run command will run the application.

## Installing credentials

You may already have credentials set up in your shared credentials file with sufficient permissions to make calls to the AWS GameLift API. If so, you may skip this section. This section assumes that you have no credentials.

This project calls the Amazon GameLift APIs from code. For these calls to succeed, AWS credentials are required. To obtain credentials, create an IAM user with the following policy, either attaching the policy to the user, or placing the user in a group which has the policy attached.

```json
{
"Version": "2012-10-17",
"Statement":
  { 
    "SID": "amazon-gamelift-remote-plus",
    "Effect": "Allow", 
    "Action": [ 
        "gamelift:DeleteAlias",
        "gamelift:DeleteBuild",
        "gamelift:DeleteFleet",
        "gamelift:DescribeFleetAttributes",
        "gamelift:DescribeFleetCapacity",
        "gamelift:DescribeInstances",
        "gamelift:GetInstanceAccess",
        "gamelift:ListAliases",
        "gamelift:ListBuilds",
        "gamelift:UpdateFleetCapacity",
        "gamelift:UpdateFleetPortSettings" ], 
    "Resource": "*" 
  }
}
```

You may choose to prohibit deletion of aliases, builds, and/or fleets by removing the respective actions in the policy, if you only want to use the tool for connecting to instances.

Create access keys for the IAM user that you created

1. Sign in to the AWS Management Console and open the IAM console at <https://console.aws.amazon.com/iam/>.

1. In the navigation pane, choose Users.

1. Choose the name of the user whose access keys you want to create, and then choose the Security credentials tab.

1. In the Access keys section, choose Create access key.

1. To view the new access key pair, choose Show. You will not have access to the secret access key again after this dialog box closes. Your credentials will look something like this:

    ```txt
    Access key ID: AKIAIOSFODNN7EXAMPLE
    Secret access key: wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY
    ```

1. To download the key pair, choose Download .csv file. Store the keys in a secure location. You will not have access to the secret access key again after this dialog box closes.

1. Keep the keys confidential in order to protect your AWS account and never email them. Do not share them outside your organization, even if an inquiry appears to come from AWS or Amazon.com. No one who legitimately represents Amazon will ever ask you for your secret key.

1. After you download the .csv file, choose Close. When you create an access key, the key pair is active by default, and you can use the pair right away.

1. For general use, the ```aws configure``` command is the fastest way to set up your AWS credentials. It uses the AWS CLI. If you have not already, install the AWS CLI v2 as described in [this section of the documentation](https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2-windows.html)

    When you enter the ```aws configure``` command, the AWS CLI prompts you for four pieces of information:

    ```txt
    Access key ID
    Secret access key
    AWS Region
    Output format
    ```

    The AWS CLI stores this information in a profile (a collection of settings) named default in the credentials file. By default, the information in this profile is used when you run an AWS CLI command that doesn't explicitly specify a profile to use. For more information on the credentials file, see Configuration and credential file settings

    The following example shows sample values. Replace them with your own values.

    ![aws configure](doc/img/awsconfigure.png)

    Default region name is not required, but if supplied, amazon-gamelift-remote-plus uses the default region to set up the region selection box when it starts, meaning that you are viewing the fleets in this region without having to select it. This is great in development when the development environment spans a single region.

## User guide - simples!

![userguide](doc/img/Userguide.png)

## Features

1. Users can select a region and see all the fleets in the region. Fleets are shown with their fleet id, name, and status.

1. Selecting a fleet permits the user one-click access to the fleet in the AWS console, using the 'View in Console' button.

1. Selecting a fleet permits the user one-click deletion of the fleet, using the 'Delete Fleet' button. The 'Delete Fleet' button is greyed out initially, and may be enabled by means of a setting in the Settings window.

1. Selecting a fleet permits opening of a scaling window. The scaling window enables the minimum, desired and maximum number of instances to be set easily using a slider control.

1. When a fleet is selected, the current scaling parameters (min, max, desired, idle, active), and a list of instances in the fleet is shown. Fleet instances are shown with their instance id, IP address, and status.

1. Selecting a fleet instance permits the user one-click connection to the fleet instance. If necessary, PuTTY is downloaded and installed, the correct ports are opened on the fleet instance to the user's local machine, access credentials are obtained for the instance from GameLift API, the credentials are converted into a usable format, and the correct tool is opened to provide remote access to the instance. This saves users a lot of time over using the command line.

1. A setting is provided to open an additional remote debugging port on the fleet if enabled. Check the box and type in a port number. When connecting to an instance, the port specified will be opened, as well as port 22 (for connecting to Linux instances with SSH) or port 3389 (for connecting to Windows instances with Remote Desktop).

1. A setting is provided to provide a custom CIDR to open ports for, in the event that a proxy or VPN is in use, for example.

1. Aliases in the region may also be shown in a simple drop down list by clicking on it.

1. Selecting an alias permits the user one-click access to the alias in the AWS console, using the 'View in Console' button.

1. Selecting an alias permits the user one-click deletion of the alias, using the 'Delete Alias' button. The 'Delete Alias' button is greyed out initially, and may be enabled by means of a setting in the Settings window.

1. Builds in the region may also be shown in a simple drop down list by clicking on it.

1. Selecting a build permits the user one-click access to the build in the AWS console, using the 'View in Console' button. The user is also permitted one-click access to create a fleet from the build in the AWS console, using the 'Create Fleet' button.

1. Selecting a build permits the user one-click deletion of the build, using the 'Delete Build' button. The 'Delete Alias' button is greyed out initially, and may be enabled by means of a setting in the Settings window.

1. When closing the application, the user is offered the option to close any ports on any fleets that were opened by the utility.

1. Any button that is greyed out (not enabled) has an instructional tooltip explaining what is needed.
