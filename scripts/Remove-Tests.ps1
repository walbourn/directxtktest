<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Removes all UWP test applications.

.LINK
https://github.com/microsoft/DirectXTK/wiki

#>

$directxtktests = @(
"db4076ae-14fb-4c16-95ea-b2894887ac6d",
"c8bf4ff0-2f8a-4254-9356-aa8f45ad8c7c",
"e18bb61b-c11e-4ac3-ba58-2d4abb4155fd",
"77504466-b4c0-44e3-a797-9bfc13334e85",
"ce2cb3e1-7586-4694-8623-672cc2988d7d",
"61adde96-80cc-4f86-95ae-40ac6659ac10",
"0978a727-17ec-42c5-b68d-8550f48717ea",
"8835D5FF-EA1B-40E0-A18E-5452E3E1EA69",
"64489900-f70d-4518-a754-30beeebc31cf",
"ff9f715c-e259-4e24-bdaa-bade5c02a44f",
"32F232AC-394B-4B7E-AF4E-F70084420491",
"09dda0bc-93bf-4bf8-a9db-38694ad9a361",
"3490dfd2-c964-4984-9a13-54d5697d971a",
"472ff739-cad7-4812-9a22-da579bfca412",
"251dee54-249d-41e0-8a41-90541f93dc40",
"2dc42269-b4ba-4b88-b7cf-6a8528a3792e",
"6eea623d-853c-4713-b7f2-22b44c877554",
"e3dbe4dc-54a3-4978-afe6-7bf1c27f0f41",
"eb13bc35-651b-406c-95bf-bc3c35c9b053",
"599ce94d-393a-471a-8511-4c6217ae6cc4"	
)

$directxtktests | % { Get-AppxPackage -Name $_ } | Remove-AppxPackage
