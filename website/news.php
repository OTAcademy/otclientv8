<?php

$news = array();
$lang = "en";
if(isset($_GET['lang']))
	$lang = $_GET['lang'];

$jokes = array();
$jokes[] = "How do you make a tissue dance? You put a little boogie in it.";
$jokes[] = "Why did the policeman smell bad? He was on duty.";
$jokes[] = "Why does Snoop Dogg carry an umbrella? FO DRIZZLE!";
$jokes[] = "Why can't you hear a pterodactyl in the bathroom? Because it has a silent pee.";
$jokes[] = "What did the Zen Buddist say to the hotdog vendor? Make me one with everything.";
$jokes[] = "What kind of bees make milk instead of honey? Boobies.";
$jokes[] = "Horse walks into a bar. Bartender says, 'Why the long face?'";
$jokes[] = "A mushroom walks into a bar. The bartender says, 'Hey, get out of here! We don't serve mushrooms here'. Mushroom says, 'why not? I'm a fungai!'";
$jokes[] = "I never make mistakes…I thought I did once; but I was wrong.";
$jokes[] = "What's Beethoven's favorite fruit? Ba-na-na-naaa!";
$jokes[] = "What did the little fish say when he swam into a wall? DAM!";
$jokes[] = "Knock knock. Who's there? Smell mop. (finish this joke in your head)";
$jokes[] = "Where does a sheep go for a haircut? To the baaaaa baaaaa shop!";
$jokes[] = "What does a nosey pepper do? Gets jalapeno business!";
$jokes[] = "Your mom is so poor, she even can't pay attention";

$news[] = array("title" => "HOW TO LOGIN", "text" => "IP: otclient.ovh\nPORT: 7171\n\nAccs:\nacc1/acc\nacc2/acc\nacc3/acc");

$news[] = array("title" => "First title", 
	"text" => "This is example of lua g_http api. Those news are from http://otclient.ovh/news.php
	\nRequest was for language '".$lang."', however, there's only english version of this, don't have time to create more versions");
$news[] = array("title" => "Title 1", "text" => "Text News no. 1.\nAny idea what to add there?\nBetter font for titles will be added soon");
$news[] = array("title" => "Title 2", "text" => "News no. 2.\nWill add something more in news 4.");
$news[] = array("title" => "Random joke", "text" => $jokes[array_rand($jokes)]);
$news[] = array("image" => "http://otclient.ovh/news.png");
$news[] = array("title" => "Title 4 with date!\n[12:11 05.03.2019]", "text" => "Ok, here is news 3.\nI am still working on new walking system\n
It's all in lua so it can be easy customized in the future. ;o\n\nClient login can be also made trought http api, like in tibia 12\nIt support also https and can even download fucking images xD");

$scroll = "";
for($i = 0; $i < 10; $i++)
	$scroll .= "lorem ipsum otclient blabla, just a lot of text blabla\n";
$news[] = array("title" => "Testing scroll", "text" => $scroll);

echo json_encode($news);

?>