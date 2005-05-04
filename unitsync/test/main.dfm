object Form1: TForm1
  Left = 192
  Top = 107
  Width = 870
  Height = 600
  Caption = 'Form1'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnActivate = FormActivate
  PixelsPerInch = 96
  TextHeight = 13
  object Button1: TButton
    Left = 52
    Top = 84
    Width = 75
    Height = 25
    Caption = 'Sync'
    TabOrder = 0
    OnClick = Button1Click
  end
  object Memo1: TMemo
    Left = 228
    Top = 36
    Width = 185
    Height = 89
    Lines.Strings = (
      'Memo1')
    TabOrder = 1
  end
  object Button2: TButton
    Left = 52
    Top = 52
    Width = 75
    Height = 25
    Caption = 'Process'
    TabOrder = 2
    OnClick = Button2Click
  end
  object Memo2: TMemo
    Left = 52
    Top = 212
    Width = 185
    Height = 89
    Lines.Strings = (
      'armaap 112647 60627 64753 armaas '
      '141880 54711 77785 armaca 124299 '
      '29332 33847 armack 123774 76166 '
      '80399 armacsub 126981 48617 '
      '46622 armacv 121807 46519 45880 '
      'armah 133039 51854 36395 armalab '
      '104186 61573 44401 armamb '
      '113971 55712 64900 armamd '
      '114660 55757 53738 armamph '
      '138290 103476 68539 armanac '
      '129733 53759 40932 armanni '
      '122467 88713 37665 armap 110410 '
      '63995 70695 armarad 103916 47321 '
      '35804 armaser 116391 57228 99526 '
      'armason 96311 45823 48271 armasp '
      '109252 26404 36994 armasy 112753 '
      '45313 4294965221 armatl 123723 '
      '18999 59911 armatlas 118227 48562 '
      '36478 armavp 109796 63810 57047 '
      'armawac 114386 16000 14986 '
      'armbats 137511 76762 55345 '
      'armbeac 98859 24508 76810 '
      'armbrawl 142303 53059 30716 '
      'armbrtha 129578 26458 56680 '
      'armbull 137398 38964 33109 armca '
      '123573 28498 30208 armcarry '
      '118922 29126 75079 armch 123784 '
      '53886 69142 armck 121944 66129 '
      '64888 armckfus 102168 14285 '
      '101781 armcom 152887 50755 '
      '54130 armcroc 130036 27049 38763 '
      'armcrus 140247 62655 64198 armcs '
      '119707 55543 76710 armcsa 129631 '
      '44390 20795 armcv 119785 45727 '
      '38929 armdecom 147449 53890 '
      '54130 armdev1 95504 23383 78682 '
      'armdrag 87785 1093 14037 armemp '
      '121202 50981 56043 armestor '
      '102360 11133 65338 armfark 126300 '
      '159225 26230 armfast 132451 66162 '
      '53427 armfav 134129 28861 51291 '
      'armfdrag 93288 1560 21639 armfhlt '
      '126857 33238 61431 armfido 122660 '
      '68091 61486 armfig 129618 45100 '
      '73643 armflak 121088 34603 44465 '
      'armflash 126793 38415 32989 '
      'armflea 130356 67001 43690 armfmkr '
      '100334 23030 71526 armfort 89619 '
      '1560 4294965159 armfrt 121608 '
      '29780 55398 armfus 107865 10357 '
      '73460 armgate 94010 29710 60776 '
      'armgeo 98650 14078 94484 '
      'armguard 123065 32715 57189 '
      'armham 126571 76609 48162 '
      'armhawk 126891 37536 68256 armhlt '
      '115998 25261 59780 armhp 117473 '
      '60931 109950 armjam 117597 18529 '
      '50753 armjeth 125257 81358 94405 '
      'armlab 102092 69311 62118 '
      'armlance 128737 37822 39552 '
      'armlatnk 129472 36254 48978 armllt '
      '108575 25984 43599 armmakr '
      '103182 19998 36030 armmanni '
      '135367 31911 78178 armmark '
      '118116 45127 41869 armmart '
      '133010 31650 52926 armmav '
      '137574 87167 87038 armmerl '
      '135422 42999 52997 armmex 98866 '
      '22456 47900 armmfus 102905 37847 '
      '68029 armmh 134648 55470 42306 '
      'armmine1 115366 1560 16556 '
      'armmine2 116179 1560 13074 '
      'armmine3 115925 1560 9927 '
      'armmine4 116057 1560 7111 '
      'armmine5 112491 1560 23771 '
      'armmine6 116487 1560 16938 '
      'armmlv 120982 54686 55746 '
      'armmmkr 103741 39892 105047 '
      'armmoho 108015 22916 38292 '
      'armmship 139153 69258 66511 '
      'armmstor 108954 10555 50991 '
      'armpeep 111803 29995 30732 '
      'armplat 118296 55218 113054 '
      'armpnix 136259 49991 26120 armpt '
      '123756 57441 61367 armpw 124972 '
      '77905 73363 armrad 94958 23952 '
      '51816 armrl 108608 28701 70973 '
      'armrock 127084 68808 50787 armroy '
      '128074 61446 60007 armsam '
      '126261 31528 46694 armscab '
      '129196 101228 99082 armscorp '
      '122283 216690 78396 armscram '
      '123569 18438 33684 armseap '
      '148969 45883 35671 armseer '
      '111979 16986 53910 armsehak '
      '121832 43685 29162 armsfig 135429 '
      '46143 24410 armsh 135468 35403 '
      '30286 armsilo 112056 45387 64511 '
      'armsjam 120574 23301 56887 '
      'armsnipe 137932 106502 105210 '
      'armsolar 100141 44980 27150 '
      'armsonar 100522 22108 91790 '
      'armspid 129364 63331 70336 armspy '
      '120909 110257 91999 armss 125288 '
      '241078 75522 armstump 128211 '
      '29133 42090 armsub 131858 18510 '
      '29226 armsubk 137501 18508 33879 '
      'armsy 103898 62415 61162 armtarg '
      '114290 58395 116396 armthovr '
      '129435 63523 58456 armthund '
      '124664 37616 60912 armtide 100274 '
      '20151 48171 armtl 113148 17631 '
      '73746 armtship 119013 53991 60484 '
      'armuwes 99707 10751 40267 '
      'armuwfus 102861 8613 90280 '
      'armuwmex 102947 20136 38556 '
      'armuwms 98397 14214 25063 '
      'armvader 124924 28519 17640 '
      'armvp 108467 60123 39056 armvulc '
      '118209 53507 67022 armwar 140784 '
      '135237 77094 armwin 94110 41087 '
      '47930 armyork 136828 41805 47095 '
      'armzeus 127542 73669 84482 '
      'coraap 112861 79858 97942 coraca '
      '124517 40463 66139 corack 123925 '
      '87595 71765 coracsub 126837 '
      '44536 73114 coracv 122021 43929 '
      '54427 corah 133359 65844 84173 '
      'corak 117914 84180 38750 coralab '
      '105721 73843 71354 coramph '
      '133416 78188 65732 corap 111405 '
      '55821 105805 corape 147890 72848 '
      '59227 corarad 102534 21230 60730 '
      'corarch 144007 77704 96476 '
      'corason 95051 19441 38570 corasp '
      '109711 26007 58939 corasy 108'
      '53851 95726 corcsa 129671 38955 '
      '38713 corcv 120305 42004 62709 '
      'cordecom 147839 55332 46471 '
      'cordev1 95663 23383 79017 '
      'cordoom 131650 122480 76798 '
      'cordrag 88504 1093 11601 corestor '
      '102356 13638 35901 coreter 118819 '
      '18216 47043 corfast 131887 70760 '
      '81554 corfav 109731 26442 49886 '
      'corfdrag 95026 1560 5141 corfhlt '
      '129064 33438 74625 corfink 112421 '
      '32803 56412 corflak 119043 34381 '
      '73801 corfmd 109603 40194 62477 '
      'corfmkr 103636 47685 73574 corfort '
      '89237 1093 3424 corfrt 120309 '
      '36470 47781 corfus 107242 19625 '
      '38940 corgant 111180 141378 '
      '88903 corgate 95060 24390 72061 '
      'corgator 127310 30716 42104 '
      'corgeo 99167 10353 62817 corgol '
      '141277 31551 47205 corhlt 115067 '
      '34998 43829 corhp 117491 57835 '
      '57062 corhrk 139729 66114 83742 '
      'corhunt 122822 47237 24908 '
      'corhurc 139342 60896 42500 corint '
      '129242 26476 46167 corkrog '
      '136801 185953 70381 corlab 103078 '
      '82534 106590 corlevlr 129576 30610 '
      '33476 corllt 116623 22959 56063 '
      'cormabm 129746 58133 62083 '
      'cormakr 98339 20707 47549 cormart '
      '138357 32547 47957 cormex 98994 '
      '24773 53467 cormh 133684 59357 '
      '77185 cormine1 114412 1526 23075 '
      'cormine2 110667 1526 18163 '
      'cormine3 112965 1526 10677 '
      'cormine4 112712 1526 27094 '
      'cormine5 114069 1526 28879 '
      'cormine6 112147 1526 15444 cormist '
      '124339 45970 68783 cormlv 121506 '
      '53546 42646 cormmkr 104646 41052 '
      '117291 cormoho 108268 41927 '
      '53213 cormort 127078 65558 38395 '
      'cormship 140646 85412 79749 '
      'cormstor 108652 10586 74707 '
      'cornecro 137243 132519 62238 '
      'corplas 94062 22164 37981 corplat '
      '116893 64119 61890 corpt 122493 '
      '56492 51130 corpun 117722 32979 '
      '66576 corpyro 134408 81113 81256 '
      'corrad 97243 19475 36548 corraid '
      '130157 32582 58319 correap '
      '137655 37718 54523 corrl 109328 '
      '32547 47085 corroach 131256 '
      '45424 30671 corroy 139215 63208 '
      '87955 corscorp 131397 216690 '
      '78396 corseal 132195 38608 51143 '
      'corseap 150182 45619 33930 '
      'corsent 140578 37659 48262 corsfig '
      '133983 46128 23984 corsh 135761 '
      '37862 58474 corshad 110174 34011 '
      '52222 corshark 134162 17383 40024 '
      'corsilo 110578 60404 60549 corsjam '
      '122263 23301 78953 corsnap '
      '128240 38852 62145 corsolar 98847 '
      '59837 59075 corsonar 98818 19224 '
      '69978 corspec 118178 55693 70194 '
      'corspy 118823 56640 60986 corss '
      '123323 241078 75522 corssub '
      '139701 47193 72090 corstorm '
      '127950 58184 71239 corsub 124037 '
      '15613 29391 corsumo 132101 94338 '
      '67374 corsy 103777 60321 67955 '
      'cortarg 114515 44315 62662 '
      'corthovr 131442 57371 61511 '
      'corthud 126739 70274 55367 cortide '
      '98921 19446 73085 cortitan 128571 '
      '39965 58769 cortl 109948 21250 '
      '95697 cortoast 113040 65841 44068 '
      'cortron 118236 43243 55361 '
      'cortruck 110454 8472 23286 cortship '
      '122623 52951 50045 coruwes '
      '100424 11620 92259 coruwfus '
      '105860 9195 99827 coruwmex '
      '102417 21815 65352 coruwms '
      '99364 10353 65104 corvalk 120849 '
      '56190 71749 corvamp 130374 43846 '
      '45294 corveng 126423 44024 46275 '
      'corvipe 117088 64328 62781 corvoyr '
      '120162 55908 68773 corvp 107675 '
      '78884 97364 corvrad 112750 15741 '
      '59536 corvroc 135698 36447 51882 '
      'corwin 94668 25070 42196 flatbridge '
      '128192 180549 80907 x1corminifus '
      '104953 26435 28649 '
      'x1daarmflaknaval 131794 36702 '
      '56939 x1eacorflaknaval 128400 '
      '34973 64207 x1ebcorjtorn 102008 '
      '29231 61729 x1ecarmjtorn 95249 '
      '25119 63254 zzz 79959 32662 '
      '23453 ')
    TabOrder = 3
  end
  object Button3: TButton
    Left = 56
    Top = 308
    Width = 75
    Height = 25
    Caption = 'Add client'
    TabOrder = 4
    OnClick = Button3Click
  end
  object Memo3: TMemo
    Left = 56
    Top = 348
    Width = 185
    Height = 89
    TabOrder = 5
  end
  object Edit1: TEdit
    Left = 144
    Top = 308
    Width = 121
    Height = 21
    TabOrder = 6
    Text = '2'
  end
  object Button4: TButton
    Left = 56
    Top = 452
    Width = 75
    Height = 25
    Caption = 'Get diff'
    TabOrder = 7
    OnClick = Button4Click
  end
  object Edit2: TEdit
    Left = 148
    Top = 452
    Width = 121
    Height = 21
    TabOrder = 8
    Text = '1'
  end
  object Button5: TButton
    Left = 488
    Top = 92
    Width = 75
    Height = 25
    Caption = 'Get unitname'
    TabOrder = 9
    OnClick = Button5Click
  end
  object Edit3: TEdit
    Left = 584
    Top = 96
    Width = 121
    Height = 21
    TabOrder = 10
    Text = '10'
  end
  object Edit4: TEdit
    Left = 584
    Top = 68
    Width = 121
    Height = 21
    TabOrder = 11
    Text = 'Edit4'
  end
  object Button6: TButton
    Left = 56
    Top = 488
    Width = 75
    Height = 25
    Caption = 'Install diff'
    TabOrder = 12
    OnClick = Button6Click
  end
  object Memo4: TMemo
    Left = 384
    Top = 356
    Width = 185
    Height = 89
    Lines.Strings = (
      'Memo4')
    TabOrder = 13
  end
  object Button7: TButton
    Left = 388
    Top = 456
    Width = 75
    Height = 25
    Caption = 'Show disabled'
    TabOrder = 14
    OnClick = Button7Click
  end
end
